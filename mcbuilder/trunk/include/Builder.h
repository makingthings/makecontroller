#ifndef BUILDER_H
#define BUILDER_H

#include <QProcess>
#include <QFileInfo>
#include "MainWindow.h"

class MainWindow;

class Builder : public QProcess
{
	Q_OBJECT
	public:
		Builder(MainWindow *mainWindow);
		void build(QString projectName);
    void clean(QString projectName);
  
	private:
		MainWindow *mainWindow;
    QString errMsg, outputMsg;
    QString currentProjectPath;
		enum BuildStep { BUILD, CLEAN, SIZER };
		BuildStep buildStep;
    int maxsize;
		void wrapFile(QString filePath);
    void resetBuildProcess();
    void filterOutput(QString output);
    void filterErrorOutput(QString errOutput);
    void sizer();
		
	private slots:
		void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
		void readOutput();
		void readError();
};

#endif // BUILDER_H