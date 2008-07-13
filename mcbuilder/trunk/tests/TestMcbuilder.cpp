

#include <QtTest/QtTest>
#include <QDirIterator>
#include <QDomDocument>
#include "ProjectManager.h"

/*
 Our test class declaration.
 each slot is automatically called as a test function.
*/
class TestMcbuilder : public QObject
{
	Q_OBJECT
  ProjectManager projectManager;
  QDir testDir;
private slots:
  void initTestCase();
  void newProject();
  void newProjectWithSpaces();
  void newFile();
  
private:
  void rmDirRecursive(QString path);
  bool inProjectFile(QString projectpath, QString filepath);
};

/*
  Utility for deleting file contents recursively
*/
void TestMcbuilder::rmDirRecursive(QString path)
{
  QDirIterator it(path, QDirIterator::Subdirectories);
  QDir dir(path);
  while(it.hasNext())
  {
    QFileInfo entry(it.next());
    if(entry.isDir() && !entry.filePath().endsWith("..") && !entry.filePath().endsWith("."))
    {
      // remove all the files, then remove the directory once it's empty
      QFileInfoList fiList = QDir(entry.filePath()).entryInfoList(QDir::Files);
      foreach(QFileInfo fi, fiList)
        dir.remove(fi.filePath());
      dir.rmdir(entry.filePath());
    }
  }
}

/*
  Confirm a given file is in a project file.
*/
bool TestMcbuilder::inProjectFile(QString projectpath, QString filepath)
{
  QDomDocument doc;
  QDir projDir(projectpath);
  QString projectName = projDir.dirName();
  QFile projectFile(projDir.filePath(projectName + ".xml"));
  bool foundfile = false;
  if(doc.setContent(&projectFile))
  {
    QDomNodeList files = doc.elementsByTagName("files").at(0).childNodes();
    for( int i = 0; i < files.count(); i++)
    {
      QString file = files.at(i).toElement().text();
      if(files.at(i).toElement().text() == filepath)
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
void TestMcbuilder::initTestCase()
{
  QDir currentDir = QDir::current();
  currentDir.cdUp();
  QDir::setCurrent(currentDir.path()); // so we can access resources in the source tree as if we were where the normal app is
  if(currentDir.exists("tests/test_debris")) // dump the contents
    rmDirRecursive(currentDir.filePath("tests/test_debris"));
  else
    currentDir.mkpath(currentDir.filePath("tests/test_debris"));
  testDir.setPath(QDir::current().filePath("tests/test_debris"));
}

/*
  Create a new project.
*/
void TestMcbuilder::newProject()
{
  QString projectName = "TestProject1";
  QString newProj = projectManager.createNewProject(testDir.filePath(projectName));
  QVERIFY(!newProj.isEmpty());
  QDir projDir(newProj);
  QCOMPARE(projDir.path(), testDir.filePath(projectName)); // make sure the project name is as we specified it
  QVERIFY(projDir.exists(projectName + ".c")); // make sure the source stub is created
  QVERIFY(projDir.exists(projectName + ".xml")); // make sure the project file is created
  
  // now confirm the source file is included appropriately in the project file
  QVERIFY(inProjectFile(projDir.path(), projectName + ".c"));
}

/*
  Confirm that the project filters spaces out of the project name appropriately.
*/
void TestMcbuilder::newProjectWithSpaces()
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
void TestMcbuilder::newFile()
{
  projectManager.createNewFile(testDir.filePath("TestProject1"), testDir.filePath("TestProject1/testfile.c"));
  QDir dir(testDir.filePath("TestProject1"));
  QVERIFY(dir.exists("testfile.c"));
  QVERIFY(inProjectFile(dir.path(), "testfile.c"));
}

/*
 execute
*/
QTEST_MAIN(TestMcbuilder)
#include "TestMcbuilder.moc"




