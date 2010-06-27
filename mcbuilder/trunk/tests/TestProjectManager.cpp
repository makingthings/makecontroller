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

#include "TestProjectManager.h"
#include <QDirIterator>
#include <QDomDocument>
#include <QDebug>

/********************************************************************************
                                  UTILS
********************************************************************************/

/*
  Utility for deleting file contents recursively
*/
void TestProjectManager::rmDirRecursive(QString path)
{
  QDirIterator it(path, QDirIterator::Subdirectories);
  QDir dir(path);
  while (it.hasNext()) {
    QFileInfo entry(it.next());
    if (entry.isDir() && !entry.filePath().endsWith("..") && !entry.filePath().endsWith(".")) {
      // remove all the files, then remove the directory once it's empty
      QFileInfoList fiList = QDir(entry.filePath()).entryInfoList(QDir::Files);
      foreach (QFileInfo fi, fiList)
        dir.remove(fi.filePath());
      dir.rmdir(entry.filePath());
    }
  }
}

/*
  Confirm a given file is in a project file.
*/
bool TestProjectManager::inProjectFile(QString projectpath, QString filepath)
{
  QDomDocument doc;
  QDir projDir(projectpath);
  QString projectName = projDir.dirName();
  QFile projectFile(projDir.filePath(projectName + ".xml"));
  bool foundfile = false;
  if (doc.setContent(&projectFile)) {
    QDomNodeList files = doc.elementsByTagName("files").at(0).childNodes();
    for (int i = 0; i < files.count(); i++) {
      QString file = files.at(i).toElement().text();
      if (files.at(i).toElement().text() == filepath)
        foundfile = true;
    }
  }
  return foundfile;
}

/********************************************************************************
                                  TESTS
********************************************************************************/

/*
  Set up our tests.
  - Clear out remnants of any previous tests
  - set the app directory to the appropriate spot
*/
void TestProjectManager::initTestCase()
{
  QDir currentDir = QDir::current();
  if (currentDir.exists("tests/test_debris")) // dump the contents
    rmDirRecursive(currentDir.filePath("tests/test_debris"));
  else
    currentDir.mkpath(currentDir.filePath("tests/test_debris"));
  testDir.setPath(QDir::current().filePath("tests/test_debris"));
  QFileInfoList entries = testDir.entryInfoList(QDir::NoDotAndDotDot);
  QVERIFY(entries.count() == 0); // make sure we actually have a clean slate
}

/*
  Create a new project.
*/
void TestProjectManager::newProject()
{
  QString projectName = "TestProject1";
  QString newProj = projectManager.createNewProject(testDir.filePath(projectName));
  QVERIFY(!newProj.isEmpty());
  QDir projDir(newProj);
  QVERIFY(projDir.exists());
  QCOMPARE(projDir.path(), testDir.filePath(projectName)); // make sure the project name is as we specified it
  QVERIFY(projDir.exists(projectName + ".c")); // make sure the source stub is created
  QVERIFY(projDir.exists(projectName + ".xml")); // make sure the project file is created

  // now confirm the source file is included appropriately in the project file
  QVERIFY(inProjectFile(projDir.path(), projectName + ".c"));
}

/*
  Confirm that the project filters spaces out of the project name appropriately.
*/
void TestProjectManager::newProjectWithSpaces()
{
  QString projectName = "Test Project 2";
  QString newProj = projectManager.createNewProject(testDir.filePath(projectName));
  QDir projDir(newProj);
  projectName.remove(" "); // the spaces should be removed
  QCOMPARE(projDir.path(), testDir.filePath(projectName)); // make sure the project name is as we specified it
  QVERIFY(projDir.exists(projectName + ".c")); // make sure the source stub is created
  QVERIFY(projDir.exists(projectName + ".xml")); // make sure the project file is created
}

/*
  Confirm a new file is created and successfully added to the project file.
*/
void TestProjectManager::newFile()
{
  QFileInfo fi = projectManager.createNewFile(testDir.filePath("TestProject1"), testDir.filePath("TestProject1/testfile.c"));
  QDir dir(testDir.filePath("TestProject1"));
  QVERIFY(fi.fileName() == "testfile.c");
  QVERIFY(inProjectFile(dir.path(), "testfile.c"));
}

/*
  Confirm a file name with spaces is sanitized.
*/
void TestProjectManager::newFileWithSpaces()
{
  QString filename("spaces test 1.c");
  QDir dir(testDir.filePath("TestProject1"));
  QFileInfo fi = projectManager.createNewFile(dir.path(), dir.filePath(filename));
  filename.remove(" ");
  QVERIFY(fi.fileName() == filename);
  QVERIFY(inProjectFile(dir.path(), filename));
}

/*
  Confirm a project is copied correctly when it's saved as a new one.
*/
void TestProjectManager::saveProjectAs()
{
  QString newProj = projectManager.saveCurrentProjectAs(testDir.filePath("TestProject1"), testDir.filePath("Test Project 3"));
  QVERIFY(!newProj.isEmpty());
  QDir newProjDir(newProj);
  QCOMPARE(newProjDir.dirName(), QString("TestProject3")); // make sure spaces have been removed
  QVERIFY(newProjDir.exists("testfile.c"));
  QVERIFY(newProjDir.exists("TestProject3.c")); // make sure TestProject1.c is now TestProject3.c
  QVERIFY(newProjDir.exists("TestProject3.xml"));
  // make sure both sources are in the project file
  QVERIFY(inProjectFile(newProjDir.path(), "testfile.c"));
  QVERIFY(inProjectFile(newProjDir.path(), "TestProject3.c"));
}

/*
  Confirm a file is copied properly and added to the project file.
*/
void TestProjectManager::saveFileAs()
{
  QDir dir(testDir.filePath("TestProject1"));
  QFileInfo fi = projectManager.saveFileAs(dir.path(), dir.filePath("TestProject1.c"), dir.filePath("SavedAs.c"));
  QVERIFY(fi.fileName() == "SavedAs.c");
  QVERIFY(inProjectFile(dir.path(), "SavedAs.c"));
}

/*
  We expect a filename with no suffix to be given a .c suffix by default
*/
void TestProjectManager::saveFileAsNoSuffix()
{
  QDir dir(testDir.filePath("TestProject1"));
  QFileInfo fi = projectManager.saveFileAs(dir.path(), dir.filePath("TestProject1.c"), dir.filePath("SavedAsNoSuffix"));
  QVERIFY(fi.fileName() == "SavedAsNoSuffix.c");
  QVERIFY(inProjectFile(dir.path(), "SavedAsNoSuffix.c"));
}

/*
  Only .c or .h are acceptable suffixes - should be changed to .c by default
*/
void TestProjectManager::saveFileAsWrongSuffix()
{
  QDir dir(testDir.filePath("TestProject1"));
  QFileInfo fi = projectManager.saveFileAs(dir.path(), dir.filePath("TestProject1.c"), dir.filePath("SavedAsBadSuffix.php"));
  QVERIFY(fi.fileName() == "SavedAsBadSuffix.c");
  QVERIFY(inProjectFile(dir.path(), "SavedAsBadSuffix.c"));
}

/*
  Confirm we can successfully remove a file from the project file.
*/
void TestProjectManager::removeFromProject()
{
  QDir dir(testDir.filePath("TestProject1"));
  if (!projectManager.removeFromProjectFile(dir.path(), "SavedAsBadSuffix.c"))
    QFAIL("removeFromProjectFile() returned false");
  QVERIFY(!inProjectFile(dir.path(), "SavedAsBadSuffix.c"));
}
