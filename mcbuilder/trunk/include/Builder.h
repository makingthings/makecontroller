/*********************************************************************************

 Copyright 2008-2009 MakingThings

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
#include "BuildLog.h"
#include "Preferences.h"

#ifdef MCBUILDER_TEST_SUITE
#include "TestBuilder.h"
#endif

class MainWindow;
class ProjectInfo;
class Preferences;

class Builder : public QProcess
{
  Q_OBJECT

  #ifdef MCBUILDER_TEST_SUITE
  friend class TestBuilder;
  #endif

public:
  Builder( MainWindow *mainWindow, ProjectInfo *projInfo, BuildLog *buildLog, Preferences* prefs );
  void build(const QString & projectName);
  void clean(const QString & projectName);
  void stop();

private:
  typedef struct Library {
    QString name;
    QStringList csrc;
    QStringList cppsrc;
  } Library;
  MainWindow *mainWindow;
  ProjectInfo *projInfo;
  BuildLog *buildLog;
  Preferences* prefs;
  QString errMsg;
  QString currentProjectPath;
  QStringList cppSuffixes;
  enum BuildStep { BUILD, CLEAN };
  BuildStep buildStep;
  int maxsize;
  QString currentProcess;
  void resetBuildProcess();
  bool matchErrorOrWarning(const QString & msg);
  bool matchInFunction(const QString & msg);
  bool matchUndefinedRef(const QString & msg);
  bool parseVersionNumber( int *maj, int *min, int *bld );
  QList<Library> loadDependencies(const QString & libsDir, const QString & project);
  QStringList generateArgs(const QString & projectName);
  void getLibrarySources(const QString & libdir, Library & lib);
  int getCtrlBoardVersionNumber();
  int getAppBoardVersionNumber();

private slots:
  void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
  void filterOutput();
  void filterErrorOutput();
  void onBuildError(QProcess::ProcessError error);
};

#endif // BUILDER_H

