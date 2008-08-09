/*********************************************************************************

 Copyright 2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include "Builder.h"
#include "ConsoleItem.h"

/*
	Builder takes a project and turns it into a binary executable.
	We need to generate a Makefile based on the general Preferences 
  and Properties for this project.
*/
Builder::Builder(MainWindow *mainWindow, ProjectInfo *projInfo, BuildLog *buildLog) : QProcess( 0 )
{
  this->mainWindow = mainWindow;
  this->projInfo = projInfo;
  this->buildLog = buildLog;
  connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(filterOutput()));
  connect(this, SIGNAL(readyReadStandardError()), this, SLOT(filterErrorOutput()));
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nextStep(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onBuildError(QProcess::ProcessError)));
  
  cleanFirst = false;
  buildPending = false;
}

/*
	Prepare the build process for the given project, then fire it off.
*/
void Builder::build(QString projectName)
{
  if(cleanFirst)
  {
    cleanFirst = false;
    buildPending = true;
    return clean(projectName);
  }
  currentProjectPath = projectName;
  ensureBuildDirExists(currentProjectPath);  // make sure we have a build dir
  setWorkingDirectory(QDir(currentProjectPath).filePath("build"));
  loadDependencies(currentProjectPath);      // this loads up the list of libraries this project depends on
  createMakefile(currentProjectPath);        // create a Makefile for this project, given the dependencies
  createConfigFile(currentProjectPath);      // create a config file based on the Properties for this project
  buildStep = BUILD;
  currentProcess = "make";
  setEnvironment(QProcess::systemEnvironment());
  QString makePath = Preferences::makePath();
  if(!makePath.isEmpty() && !makePath.endsWith("/"))  // if this is empty, just leave it so the system versions are used
    makePath += "/";
  start(makePath + "make");
  QString buildmsg("***************************************************************\n");
  buildmsg += "  mcbuilder - building " + projectName + "\n";
  buildmsg += QDateTime::currentDateTime().toString("  MMM d, yyyy h:m ap") + "\n";
  buildmsg += "***************************************************************";
  buildLog->append(buildmsg);
}

/*
  Remove all the object files from the build directory.
*/
void Builder::clean(QString projectName)
{
  currentProjectPath = projectName;
  ensureBuildDirExists(currentProjectPath);
  QDir buildDir(currentProjectPath + "/build");
  setWorkingDirectory(buildDir.path());
  if(!buildDir.exists("Makefile"))
    createMakefile(currentProjectPath);
  buildStep = CLEAN;
  QStringList args = QStringList() << "clean";
  currentProcess = "make clean";
  buildLog->clear( );
  setEnvironment(QProcess::systemEnvironment());
  QString makePath = Preferences::makePath();
  if(!makePath.isEmpty() && !makePath.endsWith("/"))  // if this is empty, just leave it so the system versions are used
    makePath += "/";
  start(QDir::toNativeSeparators(makePath + "make"), args);
}

void Builder::onProjectUpdated()
{
  cleanFirst = true;
}

void Builder::stop()
{
  kill();
}

/*
  If there's no build directory within a project, create one.
*/
void Builder::ensureBuildDirExists(QString projPath)
{
  QDir dir(projPath);
  if(!dir.exists("build"))
    dir.mkdir("build");
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
    case BUILD: // the build has just completed.  check the size of the .bin
    {
      QDir dir(currentProjectPath);
      QString projectName = dir.dirName().remove(" ").toLower();
      dir.cd("build");
      dir.setNameFilters(QStringList() << "*.bin");
      QFileInfoList bins = dir.entryInfoList();
      bool success = false;
      if(bins.count())
      {
        int filesize = bins.first().size();
        if(filesize <= 256000)
        {
          mainWindow->printOutput(QString("%1.bin is %2 out of a possible 256000 bytes.").arg(projectName).arg(filesize));
          mainWindow->onBuildComplete(true);
          success = true;
        }
        else
          mainWindow->printOutputError(QString("Error - %1.bin is too big!  %2 out of a possible 256000 bytes.").arg(projectName).arg(filesize));
      }
      if(!success)
        mainWindow->onBuildComplete(false);
      break;
    }
    case CLEAN:
      mainWindow->onCleanComplete();
      if(buildPending)
      {
        buildPending = false;
        build(currentProjectPath);
      }
      break;
  }
  resetBuildProcess();
}

/*
  Reset the build process.
*/
void Builder::resetBuildProcess()
{
  errMsg.clear();
  libraries.clear();
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
    QDir projectDir(projectPath);
    QTextStream tofile(&makefile);
    tofile << "##################################################################################################" << endl;
    tofile << "#" << endl << "# This file generated automatically by mcbuilder - ";
    tofile << QDateTime::currentDateTime().toString("MMM d, yyyy h:m ap") << endl;
    tofile << "# Any manual changes made to this file will be overwritten the next time mcbuilder builds." << endl << "#" << endl;
    tofile << "##################################################################################################" << endl << endl;

    tofile << "OUTPUT = " + projectDir.dirName().toLower() << endl << endl;
    tofile << "all: $(OUTPUT).bin" << endl << endl;
    
    // read the project file in to get a list of the files we want to build, and include dirs
    QFile projectFile(projectDir.filePath(projectDir.dirName() + ".xml"));
    if(projectFile.open(QIODevice::ReadOnly|QFile::Text))
    {
      QDomDocument projectDoc;
      if(projectDoc.setContent(&projectFile))
      {
        if(projectDoc.doctype().name() == "mcbuilder_project_file")
        {
          QString projName = projectDir.dirName();
          QDir currentDir = QDir::current();
          
          // extract all the files for this project from the project file
          QStringList thmbFiles, armFiles;
          QDomNodeList allFiles = projectDoc.elementsByTagName("files").at(0).childNodes();
          for(int i = 0; i < allFiles.count(); i++)
          {
            if(allFiles.at(i).toElement().attribute("type") == "thumb")
              thmbFiles << allFiles.at(i).toElement().text();
            else if(allFiles.at(i).toElement().attribute("type") == "arm")
              armFiles << allFiles.at(i).toElement().text();
          }

          tofile << "THUMB_SRC= \\" << endl;
          // add in all the sources from the required libraries
          foreach(Library lib, libraries)
          {
            foreach(QString filepath, lib.thumb_src)
              tofile << "  " << filteredPath(filepath) << " \\" << endl;
          }
          // now extract the source files from the project file
          foreach(QString filepath, thmbFiles)
            tofile << "  " << filteredPath(filepath) << " \\" << endl;
          tofile << endl;

          tofile << "ARM_SRC= \\" << endl;
          // add in all the sources from the required libraries
          foreach(Library lib, libraries)
          {
            foreach(QString filepath, lib.arm_src)
              tofile << "  " << filteredPath(filepath) << " \\" << endl;
          }
          // add the files from the main project file.
          foreach(QString filepath, armFiles)
            tofile << "  " << filteredPath(filepath) << " \\" << endl;
          tofile << endl;

          tofile << "INCLUDEDIRS = \\" << endl;
          tofile << "  -I" << filteredPath(projectDir.path()) << " \\" << endl; // always include the project directory
          
          // add in the directories for the required libraries
          QDir libdir(QDir::current().filePath("cores/makecontroller/libraries"));
          foreach(Library lib, libraries)
            tofile << "  -I" << filteredPath(libdir.filePath(lib.name)) << " \\" << endl;
          
          QDomNodeList include_dirs = projectDoc.elementsByTagName("include_dirs").at(0).childNodes();
          for(int i = 0; i < include_dirs.count(); i++)
          {
            QString include_dir = include_dirs.at(i).toElement().text();
            tofile << "  -I" << filteredPath(include_dir) << " \\" << endl;
          }
          tofile << endl;

          // tools
          QString toolsPath = Preferences::toolsPath();
          if(!toolsPath.isEmpty() && !toolsPath.endsWith("/"))  // if this is empty, just leave it so the system versions are used
            toolsPath += "/";
          tofile << "CC=" << QDir::toNativeSeparators(toolsPath + "arm-elf-gcc") << endl;
          tofile << "OBJCOPY=" << QDir::toNativeSeparators(toolsPath + "arm-elf-objcopy") << endl;
          tofile << "ARCH=" << QDir::toNativeSeparators(toolsPath + "arm-elf-ar") << endl;
          tofile << "CRT0=" + filteredPath("cores/makecontroller/core/startup/boot.s") << endl;
          QString debug = (projInfo->debug()) ? "-g" : "";
          tofile << "DEBUG=" + debug << endl;
          QString optLevel = projInfo->optLevel();
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
          tofile << "LDSCRIPT=" + filteredPath("cores/makecontroller/core/startup/atmel-rom.ld") << endl << endl;

          // the rest is always the same...
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
    tofile << "/*****************************************************************************************" << endl << endl;
    tofile << "  config.h" << endl;
    tofile << "  Generated automatically by mcbuilder - " << QDateTime::currentDateTime().toString("MMM d, yyyy h:m ap") << endl;
    tofile << "  Any manual changes made to this file will be overwritten the next time mcbuilder builds." << endl << endl;
    tofile << "******************************************************************************************/" << endl << endl;
    
    tofile << "#ifndef CONFIG_H" << endl << "#define CONFIG_H" << endl << endl;
    
    tofile << "#include \"controller.h\"" << endl << "#include \"error.h\"" << endl << endl;
    
    tofile << "#define CONTROLLER_HEAPSIZE " << projInfo->heapsize() << endl;
    tofile << "#define FIRMWARE_NAME " << "\"" + dir.dirName() + "\"" << endl;
    int maj, min, bld;
    parseVersionNumber( &maj, &min, &bld );
    tofile << "#define FIRMWARE_MAJOR_VERSION " << maj << endl;
    tofile << "#define FIRMWARE_MINOR_VERSION " << min << endl;
    tofile << "#define FIRMWARE_BUILD_NUMBER " << bld << endl << endl;
    
    if(projInfo->includeOsc())
      tofile << "#define OSC" << endl;
    
    if(projInfo->includeUsb())
      tofile << "#define MAKE_CTRL_USB" << endl;
      
    if(projInfo->includeNetwork())
    {
      tofile << "#define MAKE_CTRL_NETWORK" << endl;
      tofile << "#define NETWORK_MEM_POOL " << projInfo->networkMempool() << endl;
      tofile << "#define NETWORK_UDP_CONNS " << projInfo->udpSockets() << endl;
      tofile << "#define NETWORK_TCP_CONNS " << projInfo->tcpSockets() << endl;
      tofile << "#define NETWORK_TCP_LISTEN_CONNS " << projInfo->tcpServers() << endl;
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
  QStringList versions = projInfo->version().split(".");
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
  QString msg;
  switch(error)
  {
    case QProcess::FailedToStart:
      msg = QString("'%1' failed to start.  It's either missing, or doesn't have the correct permissions").arg(currentProcess);
      break;
    case QProcess::Crashed:
      msg = QString("'%1' was canceled or crashed.").arg(currentProcess);
      break;
    case QProcess::Timedout:
      msg = QString("'%1' timed out.").arg(currentProcess);
      break;
    case QProcess::WriteError:
      msg = QString("'%1' reported a write error.").arg(currentProcess);
      break;
    case QProcess::ReadError:
      msg = QString("'%1' reported a read error.").arg(currentProcess);
      break;
    case QProcess::UnknownError:
      msg = QString("'%1' - unknown error type.").arg(currentProcess);
      break;
  }
  mainWindow->printOutputError("Error: " + msg);
  resetBuildProcess();
  mainWindow->onBuildComplete(false);
}

/*
  Filter the output of the build process
  and only show the most important parts to the user.
  
  Read line by line...we're only really looking for which file is being compiled at the moment.
*/
void Builder::filterOutput()
{
  // switch based on what part of the build we're performing
  switch(buildStep)
  {
    case BUILD:
    {
      QString output = readAllStandardOutput();
      buildLog->append(output);
      QTextStream outstream(&output); // use QTextStream to deal with \r\n or \n line endings for us
      QString outline = outstream.readLine();
      while(!outline.isNull())
      {
        qDebug("msg: %s", qPrintable(outline));
        QStringList sl = outline.split(" ");
        if(sl.first().endsWith("arm-elf-gcc") && sl.at(1) == "-c")
        {
          QFileInfo srcFile(sl.last());
          mainWindow->buildingNow(srcFile.baseName() + ".c");
        }
        outline = outstream.readLine();
      }
      break;
    }
    case CLEAN:
      break;
  }
}

/*
  Filter the error messages of the build process
  and only show the most important parts to the user.
  
  Try and match the output with some common patterns so we can highlight errors/warnings
  in the editor.
*/
void Builder::filterErrorOutput()
{
  switch(buildStep)
  {
    case BUILD:
    {
      errMsg += readAllStandardError();
      if(!errMsg.endsWith("\n"))
        return;
      buildLog->append(errMsg);
      QTextStream outstream(&errMsg); // use QTextStream to deal with \r\n or \n line endings for us
      QString outline = outstream.readLine();
      bool matched = false;
      while(!outline.isNull())
      {
        qDebug("err: %s", qPrintable(outline));
        QString line = outline;
        outline = outstream.readLine();
        // now try to match the output against common patterns...
        matched = matchErrorOrWarning(line);
        if(matched)
          continue;
        matched = matchInFunction(line);
        if(matched)
          continue;
        if(matched = matchUndefinedRef(line))
          continue;
        if(line.startsWith("make")) // don't need to hear anything from make
          continue;
        // last step - we didn't match anything, just print it to the console
        if(!matched)
          mainWindow->printOutputError(line);
      }
      errMsg.clear();
      break;
    }
    case CLEAN:
    {
      QString output = readAllStandardError();
      buildLog->append(output);
      mainWindow->printOutputError(output);
      break;
    }
  }
}

/*
  Try to match an output message from gcc in the form of "filepath:linenumber: error|warning: errormessage"
  If we get one, format it nicely, highlight the line, and pop it into the UI.
*/
bool Builder::matchErrorOrWarning(QString error)
{
  bool matched = false;
  QRegExp errExp("([a-zA-Z0-9\\\\/\\.:]+):(\\d+): (error|warning): (.+)");
  int pos = 0;
  while((pos = errExp.indexIn(error, pos)) != -1)
  {
    QString filepath(errExp.cap(1));
    int linenumber = errExp.cap(2).toInt();
    QString severity(errExp.cap(3));
    QString msg(errExp.cap(4));
    
    //qDebug("cap! %s: %s, %d - %s", qPrintable(severity), qPrintable(filepath), linenumber, qPrintable(msg));
    QFileInfo fi(filepath);
    ConsoleItem *item;
    QString fullmsg = QString("%1 (line %2): %3").arg(fi.fileName()).arg(linenumber).arg(msg);
    if(severity == "error")
    {
      item = new ConsoleItem(filepath, linenumber, ConsoleItem::Error);
      item->setIcon(QIcon(":/icons/error.png"));
      mainWindow->highlightLine(filepath, linenumber, ConsoleItem::Error);
    }
    else
    {
      item = new ConsoleItem(filepath, linenumber, ConsoleItem::Warning);
      mainWindow->highlightLine(filepath, linenumber, ConsoleItem::Warning);
      item->setIcon(QIcon(":/icons/warning.png"));
    }
    item->setText(fullmsg);
    mainWindow->printOutputError(item);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Try to match an output message from gcc in the form of "filepath: In function: msg"
  If we get one, format it nicely, and pop it into the UI.
*/
bool Builder::matchInFunction(QString error)
{
  bool matched = false;
  QRegExp errExp("([a-zA-Z0-9\\\\/\\.:]+): In function (.+)");
  int pos = 0;
  while((pos = errExp.indexIn(error, pos)) != -1)
  {
    QString filepath(errExp.cap(1));
    QString func(errExp.cap(2));
    
    //qDebug("cap! %s: In function %s", qPrintable(filepath), qPrintable(func));
    QFileInfo fi(filepath);
    QString fullmsg = QString("%1: In function %2").arg(fi.fileName()).arg(func);
    mainWindow->printOutputError(fullmsg);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Try to match an output message from gcc in the form of "file: undefined reference to function"
  If we get one, format it nicely, and pop it into the UI.
*/
bool Builder::matchUndefinedRef(QString error)
{
  // match output in the form of "filepath: In function: msg"
  bool matched = false;
  QRegExp errExp("(.+):.+: undefined reference to (.+)");
  int pos = 0;
  while((pos = errExp.indexIn(error, pos)) != -1)
  {
    QString filepath(errExp.cap(1));
    QString func(errExp.cap(2));
    
    //qDebug("cap! %s: In function %s", qPrintable(filepath), qPrintable(func));
    QFileInfo fi(filepath);
    QString fullmsg = QString("Error - in %1: Undefined reference to %2").arg(fi.fileName()).arg(func);
    mainWindow->printOutputError(fullmsg);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Get a list of libraries that the source files in the current project depend on.
  Get a list of the project source files, then scan each one for #include "somelib.h"
  directives and see if any of them match the libs in our libraries directory.
  Create a Library structure for each library and store it in our class member "libraries"
*/
void Builder::loadDependencies(QString project)
{
  QDir projDir(project);
  QStringList srcFiles = projDir.entryList(QStringList() << "*.c" << "*.h");
  QDir libDir(QDir::current().filePath("cores/makecontroller/libraries"));
  QStringList libDirs = libDir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
  
  foreach(QString filename, srcFiles)
  {
    QFile file(projDir.filePath(filename));
    if(file.open(QIODevice::ReadOnly|QFile::Text))
    {
      // match anything in the form of #include "*.h" or <*.h>
      QRegExp rx("#include [\"|<]([a-zA-Z0-9]*)\\.h[\"|>]");
      QString fileContents = file.readAll();
      int pos = 0;
      
      // gather all the matching directives
      while((pos = rx.indexIn(fileContents, pos)) != -1)
      {
        QString match(rx.cap(1));
        //qDebug("match: %s", qPrintable(match));
        // only list it as a dependency if it's in our list of libraries
        if(libDirs.contains(match))
        {
          Library lib;
          lib.name = match;
          // extract the lists of source files specified in the library's spec file
          getLibrarySources( libDir.filePath(match), &lib.thumb_src, &lib.arm_src);
          libraries.append(lib);
        }
        pos += rx.matchedLength(); // step the index past the match so we can continue looking
      }
      file.close();
    }
  }
}

/*
  Read a library's spec file to determine its source files
  and append those to the lists of THUMB and ARM files passed in.
*/
void Builder::getLibrarySources(QString libdir, QStringList *thmb, QStringList *arm)
{
  QDir dir(libdir);
  QFile libfile(dir.filePath(dir.dirName() + ".xml"));
  QDomDocument libDoc;
  if(libDoc.setContent(&libfile))
  {
    QDomNodeList files = libDoc.elementsByTagName("files").at(0).childNodes();
    for(int i = 0; i < files.count(); i++)
    {
      if(files.at(i).toElement().attribute("type") == "thumb")
        thmb->append(dir.filePath(files.at(i).toElement().text()));
      else if(files.at(i).toElement().attribute("type") == "arm")
        arm->append(dir.filePath(files.at(i).toElement().text()));
    }
    libfile.close();
  }
}

/*
  Filter a path for inclusion in a Makefile.
  Make sure the directory separators are system appropriate.
  If a file path is not absolute
    1. check to see if it's in our cores dir
    2. assume it's relative to the current project
*/
QString Builder::filteredPath(QString path)
{
	// would be good to be able to do something about file paths with spaces
	// but not quite sure how to deal at the moment...
  QString filtered = path;
  if(!QDir::isAbsolutePath(path))
  {
    if(path.startsWith("cores"))
      filtered = QDir::current().filePath(path);
    else
      filtered = QDir(currentProjectPath).filePath(path);
  }
  return QDir::toNativeSeparators(filtered);
}




