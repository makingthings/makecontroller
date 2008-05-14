
#include <QDir>
#include <QTextStream>
#include "Builder.h"

/*
	Builder takes a project and turns it into a binary executable.
	We need to wrap the project into a class, and generate a Makefile
	based on the general Preferences and Properties for this project.
*/
Builder::Builder(MainWindow *mainWindow) : QProcess( 0 )
{
	this->mainWindow = mainWindow;
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(this, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nextStep(int, QProcess::ExitStatus)));
}

/*
	Prepare the build process:
	- wrap each project file in a class
	- generate lists of C and C++ files to be compiled
	Then fire it off.
*/
void Builder::build(QString projectName)
{
  buildStep = BUILD;
  currentProjectPath = projectName;
  setWorkingDirectory(projectName);
  start("make");
}

/*
  Remove all the object files from the build directory.
*/
void Builder::clean(QString projectName)
{
  buildStep = CLEAN;
  currentProjectPath = projectName;
  setWorkingDirectory(projectName);
  QStringList args = QStringList() << "clean";
  start("make", args);
}

void Builder::sizer()
{
  buildStep = SIZER;
  setWorkingDirectory(currentProjectPath + "/output");
  QStringList args = QStringList() << "heavy.elf";
  start("arm-elf-size", args);
}

void Builder::wrapFile(QString filePath)
{
  QDir dir(filePath);
	QFile project(dir.filePath(dir.dirName() + ".cpp"));
	QFile file(dir.filePath("temp.cpp"));
	if(file.open(QIODevice::WriteOnly | QFile::Text) && project.open(QIODevice::ReadOnly | QFile::Text))
	{
		QTextStream out(&file);
		out << QString("class ") + dir.dirName() << endl << "{" << endl;
		out << "  public:" << endl;
		
		QTextStream in(&project);
		QString line = in.readLine();
		while (!line.isNull())
		{
			line.append("\n");
			out << line.prepend("  ");
			line = in.readLine();
		}
		
		out << "};" << endl;
		file.close();
	}
}

/*
	This handles the end of each step of the build process.  Maintain which state we're
	in and fire off the next process as appropriate.
*/
void Builder::nextStep( int exitCode, QProcess::ExitStatus exitStatus )
{
	(void)exitStatus;
	if( exitCode != 0 ) // something didn't finish happily
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
        if(sl.at(0) == "arm-elf-gcc" && sl.at(1) == "-c")
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
      if(vals.count())
      {
        bool ok = false;
        vals.at(0).toInt(&ok);
        if(ok)
          mainWindow->printOutput(QString(".bin is %1 out of a possible 256000 bytes.").arg(vals.at(3).trimmed()));
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

        printf("err: %s\n", qPrintable(errMsg));
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





