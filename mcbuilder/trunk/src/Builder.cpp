
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include "Builder.h"

/*
	Builder takes a project and turns it into a binary executable.
	We need to generate a Makefile based on the general Preferences 
  and Properties for this project.
*/
Builder::Builder(MainWindow *mainWindow, Properties *props) : QProcess( 0 )
{
	this->mainWindow = mainWindow;
  this->props = props;
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(this, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nextStep(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onBuildError(QProcess::ProcessError)));
}

/*
	Prepare the build process:
	- generate lists of C and C++ files to be compiled
	Then fire it off.
*/
void Builder::build(QString projectName)
{
  ensureBuildDirExists(projectName);
  createMakefile(projectName);
  createConfigFile(projectName);
  buildStep = BUILD;
  currentProjectPath = projectName;
  setWorkingDirectory(projectName + "/build");
  start(Preferences::makePath() + "/make");
}

/*
  Remove all the object files from the build directory.
*/
void Builder::clean(QString projectName)
{
  ensureBuildDirExists(projectName);
  buildStep = CLEAN;
  currentProjectPath = projectName;
  setWorkingDirectory(projectName + "/build");
  QStringList args = QStringList() << "clean";
  start(Preferences::makePath() + "/make", args);
}

/*
  If there's no build directory within a project, create one.
*/
void Builder::ensureBuildDirExists(QString projPath)
{
  QDir dir(projPath);
  if(!dir.exists(projPath+"/build"))
    dir.mkdir("build");
}

/*
  Run the program that determines the size of a successfully created binary.
*/
void Builder::sizer()
{
  buildStep = SIZER;
  setWorkingDirectory(currentProjectPath + "/build");
  QDir dir(currentProjectPath);
  QStringList args = QStringList() << dir.dirName().toLower() + ".elf";
  start(Preferences::toolsPath() + "/arm-elf-size", args);
}

/*
	This handles the end of each step of the build process.  Maintain which state we're
	in and fire off the next process as appropriate.
*/
void Builder::nextStep( int exitCode, QProcess::ExitStatus exitStatus )
{
	if( exitCode != 0 || exitStatus != QProcess::NormalExit) // something didn't finish happily
	{
		mainWindow->onBuildComplete(false);
    resetBuildProcess();
		return;
	}
  
  switch(buildStep)
  {
    case BUILD:
      sizer();
      break;
    case CLEAN:
      mainWindow->onCleanComplete();
      resetBuildProcess( );
      break;
    case SIZER:
      mainWindow->onBuildComplete(true);
      resetBuildProcess( );
      break;
  }
}

/*
  Reset the build process.
*/
void Builder::resetBuildProcess()
{
  errMsg.clear();
  outputMsg.clear();
  currentProjectPath.clear();
}

/*
  Create a Makefile for this project.
  Assemble the list of source files, set the name,
  stuff in the project properties,
  add the boilerplate info and we're all set.
*/
bool Builder::createMakefile(QString projectPath)
{
  bool retval = true;
  QDir buildDir(projectPath + "/build");
  QFile makefile(buildDir.filePath("Makefile"));
  if(makefile.open(QIODevice::WriteOnly | QFile::Text))
  {
    QDir dir(projectPath);
    QTextStream tofile(&makefile);
    tofile << "##################################################################################################" << endl;
    tofile << "#" << endl << "# This file generated automatically by mcbuilder - ";
    tofile << QDateTime::currentDateTime().toString("MMM d, yyyy h:m ap") << endl;
    tofile << "# Any manual changes made to this file will be overwritten the next time mcbuilder builds." << endl << "#" << endl;
    tofile << "##################################################################################################" << endl << endl;

    tofile << "OUTPUT = " + dir.dirName().toLower() << endl << endl;
    tofile << "all: $(OUTPUT).bin" << endl << endl;

    QFile projectFile(dir.filePath(dir.dirName() + ".xml"));
    if(projectFile.open(QIODevice::ReadOnly|QFile::Text))
    {
      QDomDocument projectDoc;
      if(projectDoc.setContent(&projectFile))
      {
        if(projectDoc.doctype().name() == "mcbuilder_project_file")
        {
          QString projName = dir.dirName();
          dir = QDir::current();

          tofile << "THUMB_SRC= \\" << endl;
          tofile << "  ../" + projName + ".c \\" << endl;
          // now extract the source files from the project file
          QDomNodeList thumb_src = projectDoc.elementsByTagName("thumb_src").at(0).childNodes();
          for(int i = 0; i < thumb_src.count(); i++)
          {
            QString src_file = thumb_src.at(i).toElement().text();
            tofile << "  " << dir.filePath("resources/cores/makecontroller/") << src_file << " \\" << endl;
          }
          tofile << endl;

          tofile << "ARM_SRC= \\" << endl;
          QDomNodeList arm_src = projectDoc.elementsByTagName("arm_src").at(0).childNodes();
          for(int i = 0; i < arm_src.count(); i++)
          {
            QString src_file = arm_src.at(i).toElement().text();
            tofile << "  " << dir.filePath("resources/cores/makecontroller/") << src_file << " \\" << endl;
          }
          tofile << endl;

          tofile << "INCLUDEDIRS = \\" << endl;
          tofile << "  -I.. \\" << endl; // always include the project directory
          QDomNodeList include_dirs = projectDoc.elementsByTagName("include_dirs").at(0).childNodes();
          for(int i = 0; i < include_dirs.count(); i++)
          {
            QString include_dir = include_dirs.at(i).toElement().text();
            tofile << "  -I" << dir.filePath("resources/cores/makecontroller/") << include_dir << " \\" << endl;
          }
          tofile << endl;
          // check for libraries...

          // tools
          tofile << "CC=" << Preferences::toolsPath() << "/arm-elf-gcc" << endl;
          tofile << "OBJCOPY=" << Preferences::toolsPath() << "/arm-elf-objcopy" << endl;
          tofile << "ARCH=" << Preferences::toolsPath() << "/arm-elf-ar" << endl;
          tofile << "CRT0=" + dir.filePath("resources/cores/makecontroller/controller/startup/boot.s") << endl;
          QString debug = (props->debug()) ? "-g" : "";
          tofile << "DEBUG=" + debug << endl;
          QString optLevel = props->optLevel();
          if(optLevel.contains("-O1"))
            optLevel = "-O1";
          else if(optLevel.contains("-O2"))
            optLevel = "-O2";
          else if(optLevel.contains("-O3"))
            optLevel = "-O3";
          else if(optLevel.contains("-Os"))
            optLevel = "-Os";
          else
            optLevel = "-O0";
          tofile << "OPTIM=" + optLevel << endl;
          tofile << "LDSCRIPT=" + dir.filePath("resources/cores/makecontroller/controller/startup/atmel-rom.ld") << endl << endl;

          // flags
          tofile << "CFLAGS= \\" << endl;
          tofile << "$(INCLUDEDIRS) \\" << endl;
          tofile << "-Wall \\" << endl;
          tofile << "-Wextra \\" << endl;
          tofile << "-Wstrict-prototypes \\" << endl;
          tofile << "-Wmissing-prototypes \\" << endl;
          tofile << "-Wmissing-declarations \\" << endl;
          tofile << "-Wno-strict-aliasing \\" << endl;
          tofile << "-D SAM7_GCC \\" << endl;
          tofile << "-D THUMB_INTERWORK \\" << endl;
          tofile << "-mthumb-interwork \\" << endl;
          tofile << "-mcpu=arm7tdmi \\" << endl;
          tofile << "-T$(LDSCRIPT) \\" << endl;
          tofile << "$(DEBUG) \\" << endl;
          tofile << "$(OPTIM)" << endl << endl;

          tofile << "THUMB_FLAGS=-mthumb" << endl;
          tofile << "LINKER_FLAGS=-Xlinker -o$(OUTPUT).elf -Xlinker -M -Xlinker -Map=$(OUTPUT)_o.map" << endl << endl;

          tofile << "ARM_OBJ = $(ARM_SRC:.c=.o)" << endl;
          tofile << "THUMB_OBJ = $(THUMB_SRC:.c=.o)" << endl << endl;

          // rules
          tofile << "$(OUTPUT).bin : $(OUTPUT).elf" << endl;
          tofile << "\t" << "$(OBJCOPY) $(OUTPUT).elf -O binary $(OUTPUT).bin" << endl << endl;

          tofile << "$(OUTPUT).elf : $(ARM_OBJ) $(THUMB_OBJ) $(CRT0)" << endl;
          tofile << "\t" << "$(CC) $(CFLAGS) $(ARM_OBJ) $(THUMB_OBJ) -nostartfiles $(CRT0) $(LINKER_FLAGS)" << endl << endl;

          tofile << "$(THUMB_OBJ) : %.o : %.c" << endl;
          tofile << "\t" << "$(CC) -c $(THUMB_FLAGS) $(CFLAGS) $< -o $@" << endl << endl;

          tofile << "$(ARM_OBJ) : %.o : %.c" << endl;
          tofile << "\t" << "$(CC) -c $(CFLAGS) $< -o $@" << endl << endl;
                  
          tofile << "clean :" << endl;
          tofile << "\t" << "rm -f $(ARM_OBJ)" << endl;
          tofile << "\t" << "rm -f $(THUMB_OBJ)" << endl;
          tofile << "\t" << "rm -f $(OUTPUT).elf" << endl;
          tofile << "\t" << "rm -f $(OUTPUT).bin" << endl;
          tofile << "\t" << "rm -f $(OUTPUT)_o.map" << endl << endl;
        }
        else
          retval = false;
      }
      else
        retval = false;
      projectFile.close();
    }
    else
      retval = false;
    makefile.close();
  }
  else
    retval = false;
  return retval;
}

/*
  Create config.h.
  This file specifies several build conditions and is set
  up in the UI via the project properties.
*/
bool Builder::createConfigFile(QString projectPath)
{
  QDir dir(projectPath);
  QFile configFile(dir.filePath("config.h"));
  if(configFile.open(QIODevice::WriteOnly|QFile::Text))
  {
    QTextStream tofile(&configFile);
    tofile << "/****************************************************************" << endl << endl;
    tofile << "  config.h" << endl;
    tofile << "  Generated automatically by mcbuilder." << endl << endl;
    tofile << "****************************************************************/" << endl << endl;
    
    tofile << "#ifndef CONFIG_H" << endl << "#define CONFIG_H" << endl << endl;
    
    tofile << "#include \"controller.h\"" << endl << "#include \"appboard.h\"" << endl << "#include \"error.h\"" << endl << endl;
    
    tofile << "#define CONTROLLER_HEAPSIZE " << props->heapsize() << endl;
    tofile << "#define FIRMWARE_NAME " << "\"" + dir.dirName() + "\"" << endl;
    int maj, min, bld;
    parseVersionNumber( &maj, &min, &bld );
    tofile << "#define FIRMWARE_MAJOR_VERSION " << maj << endl;
    tofile << "#define FIRMWARE_MINOR_VERSION " << min << endl;
    tofile << "#define FIRMWARE_BUILD_NUMBER " << bld << endl << endl;
    
    if(props->includeOsc())
      tofile << "#define OSC" << endl;
    
    if(props->includeUsb())
      tofile << "#define MAKE_CTRL_USB" << endl;
      
    if(props->includeNetwork())
    {
      tofile << "#define MAKE_CTRL_NETWORK" << endl;
      tofile << "#define NETWORK_MEM_POOL " << props->networkMempool() << endl;
      tofile << "#define NETWORK_UDP_CONNS " << props->udpSockets() << endl;
      tofile << "#define NETWORK_TCP_CONNS " << props->tcpSockets() << endl;
      tofile << "#define NETWORK_TCP_LISTEN_CONNS " << props->tcpServers() << endl;
    }
    tofile << endl;
      
    tofile << "#define CONTROLLER_VERSION  100" << endl << "#define APPBOARD_VERSION  100" << endl << endl;
    
    tofile << "#endif // CONFIG_H" << endl;
    configFile.close();
  }
  return true;
}

/*
  Convert a version number string to 3 ints.
  We expect the version string to be in the form X.Y.Z
*/
bool Builder::parseVersionNumber( int *maj, int *min, int *bld )
{
  QStringList versions = props->version().split(".");
  bool success = true;
  if(versions.count() == 3)
  {
    bool ok = false;
    int temp;
    temp = versions.takeFirst().toInt(&ok);
    if(ok)
      *maj = temp;
    else
      success = false;
    temp = versions.takeFirst().toInt(&ok);
    if(ok)
      *min = temp;
    else
      success = false;
    temp = versions.takeFirst().toInt(&ok);
    if(ok)
      *bld = temp;
    else
      success = false;
  }
  if(versions.count() != 3 || !success) // just use the default
  {
    *maj = 0;
    *min = 1;
    *bld = 0;
  }
  return true;
}

void Builder::onBuildError(QProcess::ProcessError error)
{
  qDebug("Build error: %d", error);
//  switch(error)
//  {
//    
//  }
}

void Builder::readOutput( )
{
  filterOutput(readAllStandardOutput());
}

/*
  This gets called when the currently running program
  spits out some error info.  The strings can come in chunks, so
  wait till we get an end-of-line before doing anything.
*/
void Builder::readError( )
{
  filterErrorOutput(readAllStandardError());
}

/*
  Filter the output of the build process
  and only show the most important parts to the user.
  
  Strings can come in chunks, so wait till we get an 
  end-of-line before doing anything.
*/
void Builder::filterOutput(QString output)
{
  // switch based on what part of the build we're performing
  switch(buildStep)
  {
    case BUILD:
    {
      outputMsg += output;
      if(outputMsg.endsWith("\n")) // we have a complete message to deal with
      {
        //printf("msg: %s\n", qPrintable(outputMsg));
        QStringList sl = outputMsg.split(" ");
        if(sl.first().endsWith("arm-elf-gcc") && sl.at(1) == "-c")
        {
          QStringList sl2 = sl.last().split("/");
          mainWindow->buildingNow(sl2.last());
        }
        outputMsg.clear();
      }
      break;
    }
    case CLEAN:
      break;
    case SIZER:
    {
      QStringList vals = output.split(" ", QString::SkipEmptyParts);
      if(vals.count() == 10) // we expect 10 values back from arm-elf-size
      {
        bool ok = false;
        int total_size = vals.at(8).toInt(&ok);
        if(ok)
        {
          QDir dir(currentProjectPath);
          mainWindow->printOutput(QString("%1.bin is %2 out of a possible 256000 bytes.").arg(dir.dirName().toLower()).arg(total_size));
        }
      }
      break;
    }
  }
}

/*
  Filter the error messages of the build process
  and only show the most important parts to the user.
*/
void Builder::filterErrorOutput(QString errOutput)
{
  switch(buildStep)
  {
    case BUILD:
      errMsg += errOutput;
      if(errMsg.endsWith("\n")) // we have a complete message to deal with
      {
        QStringList sl = errMsg.split(":");
        for(int i = 0; i < sl.count(); i++) // remove any spaces from front and back
          sl[i] = sl.at(i).trimmed();
            
        //printf("err: %s\n", qPrintable(errMsg));
        if(sl.contains("warning"))
        {
          QString msg;
          QTextStream(&msg) << "Warning - " << sl.last() << endl;
          mainWindow->printOutputError(msg);
        }
        else if(sl.contains("error"))
        {
          QString msg;
          QTextStream(&msg) << "Error - " << sl.last() << endl;
          mainWindow->printOutputError(msg);
        }
        else
          mainWindow->printOutputError(errMsg);
        errMsg.clear();
      }
      break;
    case CLEAN:
      mainWindow->printOutputError(errOutput);
      break;
    case SIZER:
      mainWindow->printOutputError(errOutput);
      break;
  }
}





