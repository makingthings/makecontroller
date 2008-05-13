
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
  setWorkingDirectory(projectName);
  start("make");
}

/*
  Remove all the object files from the build directory.
*/
void Builder::clean(QString projectName)
{
  setWorkingDirectory(projectName);
  QStringList args = QStringList() << "clean";
  start("make", args);
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
		resetBuildProcess();
		return;
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
  buildStep = COMPILE_C;
}

void Builder::readOutput( )
{
  QTextStream stream(this);
  QString line;
  do
  {
    line = stream.readLine();
    //filterOutput(line);
    mainWindow->printOutput(line);
  } while (!line.isNull());
}

/*
  This gets called when the currently running program
  spits out some error info.  The strings can come in chunks, so
  wait till we get an end-of-line before doing anything.
*/
void Builder::readError( )
{
  QTextStream stream(this);
  QString line;
  do
  {
    line = stream.readLine();
    filterErrorOutput(line);
  } while (!line.isNull());
}

void Builder::filterOutput(QString output)
{
  mainWindow->printOutput(output);
}

void Builder::filterErrorOutput(QString errOutput)
{
  mainWindow->printOutputError(errOutput);
}





