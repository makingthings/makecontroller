
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
	if(!loadTools())
    return;
  //wrapFile(projectName);
  if(!loadSourceFiles(projectName))
    return;
  buildStep = COMPILE_C;
  compileCc( ); // fire off the first process
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

// read the board profile and extract which tools to use to build it
bool Builder::loadTools( )
{
  bool retval = false;
  QDomDocument doc;
  QDir dir = QDir::current();
  dir.cd("resources/board_profiles");
  QFile file(dir.filePath(mainWindow->currentBoardProfile()));
	if(!file.exists())
		return retval;
  
  if(file.open(QIODevice::ReadOnly))
	{
		if(doc.setContent(&file))
		{
			// first get the relative tools path
      QString toolsrelpath = doc.elementsByTagName("toolspath").at(0).toElement().text();
      QString toolspath = QDir::currentPath() + "/resources/tools/" + toolsrelpath + "/";
      // then all the tools
      ccTool = toolspath + doc.elementsByTagName("cc").at(0).toElement().text();
      cppTool = toolspath + doc.elementsByTagName("cpp").at(0).toElement().text();
      ldTool = toolspath + doc.elementsByTagName("ld").at(0).toElement().text();
      asTool = toolspath + doc.elementsByTagName("as").at(0).toElement().text();
      sizeTool = toolspath + doc.elementsByTagName("sizer").at(0).toElement().text();
      maxsize = doc.elementsByTagName("maxsize").at(0).toElement().text().toInt();
      retval = true;
		}
		file.close();
	}
  return retval;
}

bool Builder::loadSourceFiles( QString project )
{
  bool retval = false;
  QDir dir(project);
  cSrc += dir.entryList(QStringList() << "*.c" << "*.s" << "*.S");
  cppSrc += dir.entryList(QStringList("*.cpp"));
  if( cSrc.count() && cppSrc.count())
    retval = true;
  return retval;
}

/*
	This handles the end of each step of the build process.  Maintain which state we're
	in and fire off the next process as appropriate.
*/
void Builder::nextStep( int exitCode, QProcess::ExitStatus exitStatus )
{
	(void)exitStatus;
	// if something didn't finish happily, reset and return
	if( exitCode != 0 )
	{
		buildStep = COMPILE_C;
		return;
	}
  
	switch(buildStep)
	{
		case COMPILE_C:
      compileCc( );
      break;
		case COMPILE_CPP:
      compileCpp( );
			break;
		case LINK:
			break;
		case ASSEMBLE:
			break;
		case SIZER:
			break;
	}
}

void Builder::readOutput( )
{
	mainWindow->printOutput(readAllStandardOutput());
}

void Builder::readError( )
{
	mainWindow->printOutputError(readAllStandardError());
}

void Builder::compileCc( )
{
	QStringList args;
  QFile file(cSrc.takeFirst());
  QFileInfo fi(file);
  args << file.fileName() << "-c"; // the file name must be first, then specify to only compile, not link
  args << "-o " + fi.fileName() + ".o"; // put the object file where we can get at it
  if(!fi.fileName().endsWith("_arm") && !fi.fileName().endsWith("_isr"))
    args << "-mthumb"; // build these files as ARM, not THUMB
  if(cSrc.count() == 0) // if this was the last file, move on to the next step
    buildStep = COMPILE_CPP;
  start(ccTool, args);
}

void Builder::compileCpp( )
{
	QStringList args;
  QFile file(cppSrc.takeFirst());
  QFileInfo fi(file);
  args << file.fileName() << "-c"; // the file name must be first, then specify to only compile, not link
  args << "-o " + fi.fileName() + ".o"; // put the object file where we can get at it
  if(!fi.fileName().endsWith("_arm") && !fi.fileName().endsWith("_isr"))
    args << "-mthumb"; // build these files as ARM, not THUMB
  if(cppSrc.count() == 0) // if this was the last file, move on to the next step
    buildStep = LINK;
  start(cppTool, args);
}

void Builder::link( )
{
	QStringList args;
  start(ldTool, args);
}

void Builder::assemble( )
{
	QStringList args;
  start(asTool, args);
}

void Builder::sizer( )
{
	QStringList args;
  start(sizeTool, args);
}

// reset our build state and print relevant messages to the output console
void Builder::finish( int exitCode, QString msg )
{
  if(exitCode)
    mainWindow->printOutputError(msg);
  else
    mainWindow->printOutput(msg);
	
  cSrc.clear();
  cppSrc.clear();
  buildStep = COMPILE_C;
}




