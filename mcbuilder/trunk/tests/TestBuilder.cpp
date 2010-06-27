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

#include "TestBuilder.h"
#include "ProjectInfo.h"

#define TEST_PROJECT "resources/examples/Input-Output/AinToServo"

TestBuilder::TestBuilder(MainWindow* window)
{
  this->window = window;
}

/*
 util - get the "current" project...just our test project.
*/
QString TestBuilder::currentProjectPath()
{
  QDir proj = QDir::current().filePath(TEST_PROJECT);
  return proj.path();
}

/*
  This is run before any other tests.
*/
void TestBuilder::initTestCase()
{
  window->openProject(currentProjectPath());
  builder = window->builder;
}

/*
  Take an example project and load its libraries.
  Make sure the appropriate libraries are loaded, no more, no less.
*/
void TestBuilder::loadLibs()
{
  // change the path to libraries if you need to for your setup...not sure how best to automatically do that
  QDir libDir = QDir::cleanPath(MainWindow::appDirectory().filePath("../cores/makecontroller/libraries"));

  QList<Builder::Library> libs = builder->loadDependencies(libDir.path(), currentProjectPath());
  // the AinToServo example only pulls in one library, servo
  QCOMPARE(libs.size(), 1);
  QVERIFY(libs.first().name == QString("servo"));
  // only expect there to be one thumb src file - servo.c
  QCOMPARE(libs.first().csrc.size(), 1);
  QString servoFile = libs.first().csrc.first();
  QVERIFY(QDir::isAbsolutePath(servoFile)); // make sure the path is absolute (not relative)
  libDir.cd("servo");
  QVERIFY(libDir.exists(servoFile));
  QCOMPARE(libs.first().cppsrc.size(), 0);
}

void TestBuilder::testClean()
{
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
  QSignalSpy finishedSpy(builder, SIGNAL(finished(int, QProcess::ExitStatus)));
  QSignalSpy errorSpy(builder, SIGNAL(error(QProcess::ProcessError)));

  builder->clean(currentProjectPath());
  builder->waitForFinished();

  QCOMPARE(errorSpy.count(), 0); // make sure we didn't get any errors
  for (int i = 0; i < finishedSpy.count(); i++) {
    int exitcode = finishedSpy.at(i).at(0).toInt();
    int exitstatus = finishedSpy.at(i).at(1).toInt();
    if (exitcode != 0 || exitstatus != QProcess::NormalExit) {
      qWarning() << "exitcode" << exitcode << "exitstatus" << exitstatus;
      QFAIL("make/clean exited unhappily.");
    }
  }

  QDir projDir(currentProjectPath());
  QString shortname = projDir.dirName().toLower();
  projDir.cd("build");
  QVERIFY(!projDir.exists(shortname + ".bin"));
  QVERIFY(!projDir.exists(shortname + ".elf"));
  QVERIFY(!projDir.exists(shortname + ".map"));
}

void TestBuilder::testBuild( )
{
  QSignalSpy finishedSpy(builder, SIGNAL(finished(int, QProcess::ExitStatus)));
  QSignalSpy errorSpy(builder, SIGNAL(error(QProcess::ProcessError)));

  builder->build(currentProjectPath());
  while(builder->state() != QProcess::NotRunning) // wait until the build is complete
    QTest::qWait(100);

  QCOMPARE( errorSpy.count(), 0); // make sure we didn't get any errors
  for( int i = 0; i < finishedSpy.count(); i++ ) {
    int exitcode = finishedSpy.at(i).at(0).toInt();
    int exitstatus = finishedSpy.at(i).at(1).toInt();
    if( exitcode != 0 || exitstatus != QProcess::NormalExit ) {
      qWarning() << "exit code:" << exitcode << "exit status:" << exitstatus;
      QFAIL("make/build exited unhappily.");
    }
  }

  // now let's make sure our .bin and friends are where we expect them
  QDir projDir(currentProjectPath());
  QString shortname = projDir.dirName().toLower();
  projDir.cd("build");
  QVERIFY(projDir.exists(shortname + ".bin"));
  QVERIFY(projDir.exists(shortname + ".elf"));
  QVERIFY(projDir.exists(shortname + "_o.map"));
}


