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


#ifndef BUILDER_H
#define BUILDER_H

#include <QProcess>
#include <QFileInfo>
#include "MainWindow.h"
#include "ProjectInfo.h"

class MainWindow;
class ProjectInfo;

class Builder : public QProcess
{
  Q_OBJECT
public:
  Builder(MainWindow *mainWindow, ProjectInfo *projInfo);
  void build(QString projectName);
  void clean(QString projectName);
  void stop();
  
private:
  struct Library
  {
    QString name;
    QStringList thumb_src;
    QStringList arm_src;
  };
  MainWindow *mainWindow;
  ProjectInfo *projInfo;
  QString errMsg, outputMsg;
  QString currentProjectPath;
  enum BuildStep { BUILD, CLEAN };
  BuildStep buildStep;
  int maxsize;
  QString currentProcess;
  QList<Library> libraries;
  void resetBuildProcess();
  bool createMakefile(QString projectPath);
  bool createConfigFile(QString projectPath);
  void filterOutput(QString output);
  void filterErrorOutput(QString errOutput);
  void ensureBuildDirExists(QString projPath);
  bool parseVersionNumber( int *maj, int *min, int *bld );
  void loadDependencies(QString project);
  void getLibrarySources(QString libdir, QStringList *thmb, QStringList *arm);
  QString filteredPath(QString path);
		
private slots:
  void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
  void readOutput();
  void readError();
  void onBuildError(QProcess::ProcessError error);
};

#endif // BUILDER_H

