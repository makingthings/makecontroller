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

#ifndef TEST_PROJECT_MANAGER_H
#define TEST_PROJECT_MANAGER_H


#include "ProjectManager.h"
#include <QtTest/QtTest>
#include <QDir>

/*
 Our test class declaration.
 each slot is automatically called as a test function.
*/
class TestProjectManager : public QObject
{
  Q_OBJECT
  ProjectManager projectManager;
  QDir testDir;
private slots:
  void initTestCase();
  void newProject();
  void newProjectWithSpaces();
  void newFile();
  void newFileWithSpaces();
  void saveProjectAs();
  void saveFileAs();
  void saveFileAsNoSuffix();
  void saveFileAsWrongSuffix();
  void removeFromProject();

private:
  void rmDirRecursive(QString path);
  bool inProjectFile(QString projectpath, QString filepath);
};

#endif // TEST_PROJECT_MANAGER_H





