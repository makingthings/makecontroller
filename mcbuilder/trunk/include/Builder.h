#ifndef BUILDER_H
#define BUILDER_H

#include <QProcess>
#include "MainWindow.h"

class MainWindow;

class Builder : public QObject
{
	Q_OBJECT
	public:
		Builder(MainWindow *mainWindow);
		void build(QString projectName);
		QProcess::ProcessState state( ) { return builder.state(); }
		
	private:
		MainWindow *mainWindow;
		QProcess builder;
		QStringList cSrc, cppSrc; // lists of files to be compiled
    QString ccTool, cppTool, ldTool, asTool, sizeTool; // names of the tools to use
		enum BuildStep { COMPILE_C, COMPILE_CPP, LINK, ASSEMBLE, SIZER };
		BuildStep buildStep;
		void wrapFile(QString filePath);
		void compileCpp( );
		void compileCc( );
		void link( );
		void assemble( );
		void sizer( );
    bool loadTools( );
    bool loadSourceFiles( QString project );
		
	private slots:
		void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
		void readOutput();
		void readError();
};

#endif // BUILDER_H