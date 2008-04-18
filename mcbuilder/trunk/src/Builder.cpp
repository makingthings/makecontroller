
#include <QDir>
#include <QTextStream>
#include "Builder.h"

/*
	Builder takes a project and turns it into a binary executable.
	We need to wrap the project into a class, and generate a Makefile
	based on the general Preferences and Properties for this project.
*/
Builder::Builder(MainWindow *mainWindow) : QObject( 0 )
{
	this->mainWindow = mainWindow;
	connect(&builder, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(&builder, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(&builder, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nextStep(int, QProcess::ExitStatus)));
}

/*
	Prepare the build process:
	- wrap each project file in a class
	- generate lists of C, C (ARM), C++, and C++ (ARM) files to be compiled
	Then fire it off.
*/
void Builder::build(QString projectName)
{
	if(!loadTools())
    return;
  //wrapFile(projectName);
  if(!loadSourceFiles(projectName))
    return;
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
  dir.cd("boards");
  QFile file(dir.filePath(mainWindow->currentBoardProfile()));
	if(!file.exists())
		return retval;
  
  if(file.open(QIODevice::ReadOnly))
	{
		if(doc.setContent(&file))
		{
			ccTool = doc.elementsByTagName("cc").at(0).toElement().text();
      cppTool = doc.elementsByTagName("cpp").at(0).toElement().text();
      ldTool = doc.elementsByTagName("ld").at(0).toElement().text();
      asTool = doc.elementsByTagName("as").at(0).toElement().text();
      sizeTool = doc.elementsByTagName("sizer").at(0).toElement().text();
      if( !ccTool.isEmpty() && !cppTool.isEmpty() && !ldTool.isEmpty() && !asTool.isEmpty() && !sizeTool.isEmpty() )
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
	mainWindow->printOutput(builder.readAllStandardOutput());
}

void Builder::readError( )
{
	mainWindow->printOutputError(builder.readAllStandardError());
}

void Builder::compileCc( )
{
	QStringList args;
  QFile file(cSrc.takeFirst());
  QFileInfo fi(file);
  args << file.fileName() << "-c"; // the file name must be first, then specify to only compile, not link
  args << "-o " + fi.fileName() + ".o"; // put the object file where we can get at it
//  if(arm)
//    args << "-mthumb";
  if(cSrc.count() == 0) // if this was the last file, move on to the next step
    buildStep = COMPILE_CPP;
  builder.start(ccTool, args);
}

void Builder::compileCpp( )
{
	
}

void Builder::link( )
{
	
}

void Builder::assemble( )
{
	
}

void Builder::sizer( )
{
	
}



