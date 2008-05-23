#ifndef BUILDER_H
#define BUILDER_H

#include <QProcess>
#include <QFileInfo>
#include "MainWindow.h"
#include "Properties.h"

class MainWindow;
class Properties;

class Builder : public QProcess
{
	Q_OBJECT
	public:
		Builder(MainWindow *mainWindow, Properties *props);
		void build(QString projectName);
    void clean(QString projectName);
  
	private:
		MainWindow *mainWindow;
    Properties *props;
    QString errMsg, outputMsg;
    QString currentProjectPath;
		enum BuildStep { BUILD, CLEAN, SIZER };
		BuildStep buildStep;
    int maxsize;
    void resetBuildProcess();
    bool createMakefile(QString projectPath);
    bool createConfigFile(QString projectPath);
    void filterOutput(QString output);
    void filterErrorOutput(QString errOutput);
    void sizer();
    void ensureBuildDirExists(QString projPath);
    bool parseVersionNumber( int *maj, int *min, int *bld );
		
	private slots:
		void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
		void readOutput();
		void readError();
    void onBuildError(QProcess::ProcessError error);
};

#endif // BUILDER_H