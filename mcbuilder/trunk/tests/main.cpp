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

#include <QtTest/QtTest>
#include <QApplication>
#include "MainWindow.h"
#include "TestProjectManager.h"
#include "TestBuilder.h"
#include "TestProjectInfo.h"

/*
  A test suite that fires off each unit test in succession.
*/
int main(int argc, char* argv[])
{  
  (void)argc;
  (void)argv;
  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("MakingThings");
  QCoreApplication::setOrganizationDomain("makingthings.com");
  QCoreApplication::setApplicationName("mcbuilder");
  MainWindow window;
  
  TestProjectManager testProjectManager;
  QTest::qExec(&testProjectManager);
  
  TestBuilder testBuilder(&window);
  QTest::qExec(&testBuilder);
  
  TestProjectInfo testProjectInfo(&window);
  QTest::qExec(&testProjectInfo);
}





