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
  struct Library
  {
    QString name;
    QStringList thumb_src;
    QStringList thumb_cpp_src;
    QStringList arm_src;
    QStringList arm_cpp_src;
  };
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
  QList<Library> libraries;
  void resetBuildProcess();
  bool createMakefile(const QString & projectPath);
  bool createConfigFile(const QString & projectPath);
  bool compareConfigFile(const QString & projectPath);
  bool matchErrorOrWarning(const QString & msg);
  bool matchInFunction(const QString & msg);
  bool matchUndefinedRef(const QString & msg);
  QString ensureBuildDirExists(const QString & projPath);
  bool parseVersionNumber( int *maj, int *min, int *bld );
  void loadDependencies(const QString & libsDir, const QString & project);
  void getLibrarySources(const QString & libdir, Library & lib);
  QString filteredPath(const QString & path);
  int getCtrlBoardVersionNumber();
  int getAppBoardVersionNumber();
  void writeFileListToMakefile(QTextStream & stream, const QStringList & files);
  void writeGroupToMakeFile(QTextStream & stream, const QString & groupName, const QStringList & files);

private slots:
  void nextStep( int exitCode, QProcess::ExitStatus exitStatus );
  void filterOutput();
  void filterErrorOutput();
  void onBuildError(QProcess::ProcessError error);
};

#endif // BUILDER_H

