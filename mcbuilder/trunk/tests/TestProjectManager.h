


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
  void changeBuildType();
  
private:
  void rmDirRecursive(QString path);
  bool inProjectFile(QString projectpath, QString filepath);
};






