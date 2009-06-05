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

#ifndef TEST_PROJECT_INFO_H
#define TEST_PROJECT_INFO_H

#include <QtTest/QtTest>
#include "MainWindow.h"
#include "Builder.h"
#include "ProjectInfo.h"

class Builder;
class ProjectInfo;

/*
 Test class for Builder.cpp
*/
class TestProjectInfo : public QObject
{
  Q_OBJECT

public:
  TestProjectInfo( MainWindow* mw );

private:
  MainWindow* mainWindow;
  ProjectInfo* projectInfo;

private slots:
  void includeSystem();
};

#endif // TEST_PROJECT_INFO_H






