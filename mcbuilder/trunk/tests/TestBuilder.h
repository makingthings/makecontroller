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

#ifndef TEST_BUILDER_H
#define TEST_BUILDER_H

#include <QtTest/QtTest>
#include "MainWindow.h"
#include "Builder.h"

class Builder;

/*
 Test class for Builder.cpp
*/
class TestBuilder : public QObject
{
  Q_OBJECT

public:
  TestBuilder( MainWindow* window );
  QString currentProjectPath();

private:
  Builder* builder;
  MainWindow* window;

private slots:
  void initTestCase();
  void loadLibs();
  void testClean();
  void testBuild();
};

#endif // TEST_BUILDER_H






