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

TestBuilder::TestBuilder( MainWindow* window )
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
  QDir libDir = QDir::current().filePath("build/Debug/cores/makecontroller/libraries");
  builder->loadDependencies(libDir.path(), currentProjectPath());
  QList<Builder::Library> libs = builder->libraries;
  // the AinToServo example only pulls in one library, servo
  QVERIFY(libs.size() == 1);
  QVERIFY(libs.at(0).name == QString("servo"));
  // only expect there to be one thumb src file - servo.c
  QVERIFY(libs.at(0).thumb_src.size() == 1);
  QString servoFile = libs.at(0).thumb_src.at(0);
  QVERIFY(QDir::isAbsolutePath(servoFile)); // make sure the path is absolute (not relative)
  libDir.cd("servo");
  QVERIFY(libDir.exists(servoFile));
  QVERIFY(libs.at(0).arm_src.size() == 0);
}

/*
  Create a Makefile and verify that all the files in the project file are included,
  the appropriate include dirs are there, taking into account any loaded libraries.
*/
void TestBuilder::testMakefile()
{
  builder->createMakefile(currentProjectPath());
  QDir projDir(currentProjectPath());
  QFile makefile(projDir.filePath("build/Makefile"));
  QVERIFY(makefile.exists());

  QStringList projectFiles; // a list of all the files in the project file
  QFile projectFile(projDir.filePath(projDir.dirName() + ".xml"));
  QDomDocument projectDoc;
  if(projectDoc.setContent(&projectFile)) // read the project file and extract all the file paths
  {
    QDomNodeList files = projectDoc.elementsByTagName("files").at(0).childNodes();
    for(int i = 0; i < files.count(); i++)
      projectFiles.append(builder->filteredPath(files.at(i).toElement().text()));
  }
  QVERIFY(projectFiles.size()); // make sure we got something

  QStringList makeFiles; // list of files in the Makefile
  QStringList makeFileDirs; // list of include dirs in the Makefile
  QVERIFY(makefile.open(QFile::ReadOnly | QFile::Text));
  QTextStream in(&makefile);
  QString makeLine = in.readLine();
  static const int BEGIN = 0;
  static const int FILES = 1;
  static const int DIRS = 2;
  int state = BEGIN;
  while(!makeLine.isNull())
  {
    switch(state)
    {
      case BEGIN:
        if(makeLine.startsWith("THUMB_SRC") )
          state = FILES;
        break;
      case FILES:
        if(makeLine.startsWith("INCLUDEDIRS"))
          state = DIRS;
        else if(!makeLine.isEmpty() && !makeLine.startsWith("ARM_SRC"))
          makeFiles << makeLine.remove("\\").trimmed();
        break;
      case DIRS:
        if(makeLine.startsWith("CC"))
          state = BEGIN;
        else if(!makeLine.isEmpty())
          makeFileDirs << makeLine.remove("\\").remove("-I").trimmed();
    }
    makeLine = in.readLine();
  }
  QVERIFY(makeFiles.size()); // make sure we got something
  QVERIFY(makeFileDirs.size());

  // now, let's compare our lists
  // it's possible (probable) that the list of files in the makefiles list will be
  // longer since it will have libraries files added as part of the build process
  // that are not included in the project file.
  int matches = 0;
  int libraryFiles = 0;
  foreach(QString file, makeFiles)
  {
    if(projectFiles.contains(file))
      matches++;
    else if(file.contains("cores/makecontroller/libraries"))
      libraryFiles++;
  }
  //qDebug("makefiles: %d, projectfiles: %d, matches: %d", makeFiles.size(), projectFiles.size(), matches);
  QVERIFY( matches == (makeFiles.size() - libraryFiles));

  // now check that the appropriate include directories have been added
  QDomNodeList dirs = projectDoc.elementsByTagName("include_dirs").at(0).childNodes();
  QStringList includeDirs;
  for(int i = 0; i < dirs.count(); i++)
    includeDirs.append(builder->filteredPath(dirs.at(i).toElement().text()));
  QVERIFY(includeDirs.size()); // make sure we got something

  matches = 0;
  int libraryDirs = 0;
  foreach( QString dir, makeFileDirs )
  {
    if(includeDirs.contains(dir))
      matches++;
    else if(dir.contains("cores/makecontroller/libraries"))
      libraryDirs++;
  }
  //qDebug("makefile dirs: %d, project file dirs: %d, matches: %d, library dirs: %d",
  //         makeFileDirs.size(), includeDirs.size(), matches, libraryDirs);

  // one additional difference is that the Makefile should include the project dir itself as an include dir
  // whereas this is not specificed in the project file
  QVERIFY(matches == (makeFileDirs.size() - libraryDirs - 1));
}

/*
  Create a config file.
  Make sure it includes the appropriate elements, based on the
  ProjectInfo.
*/
void TestBuilder::testConfigFile()
{
  QDir projDir(currentProjectPath());
  QFile configFile(projDir.filePath("config.h"));

  if( configFile.exists() )
    projDir.remove("config.h"); // get rid of the file, so we can test creating it

  builder->createConfigFile(currentProjectPath());
  // we haven't changed anything in the file yet, so this should return false
  QVERIFY( builder->compareConfigFile(currentProjectPath()) == false );

  // now let's change a few things in the config file, and confirm that we need to update it
  ProjectInfo* pi = window->projInfo;
  pi->setHeapSize( pi->heapsize() + 100 );
  QVERIFY( builder->compareConfigFile(currentProjectPath()) == true );

}

void TestBuilder::testClean()
{
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
  QSignalSpy finishedSpy(builder, SIGNAL(finished(int, QProcess::ExitStatus)));
  QSignalSpy errorSpy(builder, SIGNAL(error(QProcess::ProcessError)));

  builder->clean(currentProjectPath());
  while(builder->state() != QProcess::NotRunning) // wait until the clean is complete
    QTest::qWait(100);

  QVERIFY( errorSpy.count() == 0); // make sure we didn't get any errors
  for( int i = 0; i < finishedSpy.count(); i++ )
  {
    int exitcode = finishedSpy.at(i).at(0).toInt();
    int exitstatus = finishedSpy.at(i).at(1).toInt();
    if( exitcode != 0 || exitstatus != QProcess::NormalExit )
      QFAIL("make/clean exited unhappily.");
  }

  QDir projDir(currentProjectPath());
  QString shortname = projDir.dirName().toLower();
  projDir.cd("build");
  QVERIFY(!projDir.exists(shortname + ".bin"));
  QVERIFY(!projDir.exists(shortname + ".elf"));
  QVERIFY(!projDir.exists(shortname + "_o.map"));
}

void TestBuilder::testBuild( )
{
  QSignalSpy finishedSpy(builder, SIGNAL(finished(int, QProcess::ExitStatus)));
  QSignalSpy errorSpy(builder, SIGNAL(error(QProcess::ProcessError)));

  builder->build(currentProjectPath());
  while(builder->state() != QProcess::NotRunning) // wait until the build is complete
    QTest::qWait(100);

  QVERIFY( errorSpy.count() == 0); // make sure we didn't get any errors
  for( int i = 0; i < finishedSpy.count(); i++ )
  {
    int exitcode = finishedSpy.at(i).at(0).toInt();
    int exitstatus = finishedSpy.at(i).at(1).toInt();
    if( exitcode != 0 || exitstatus != QProcess::NormalExit )
    {
      qWarning("exit code: %d, exit status: %d", exitcode, exitstatus);
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


