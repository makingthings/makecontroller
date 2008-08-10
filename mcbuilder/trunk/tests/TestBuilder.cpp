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
  builder = window->builder;
  builder->currentProjectPath = currentProjectPath();
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
  QVERIFY(makefile.open(QFile::ReadOnly | QFile::Text));
  QTextStream in(&makefile);
  QString makeLine = in.readLine();
  static const int BEGIN = 0;
  static const int FILES = 1;
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
          state = BEGIN;
        else
        {
          if(!makeLine.isEmpty() && !makeLine.startsWith("ARM_SRC"))
            makeFiles.append(makeLine.remove("\\").trimmed());
        }
        break;
    }
    makeLine = in.readLine();
  }
  QVERIFY(makeFiles.size()); // make sure we got something
  
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
}


