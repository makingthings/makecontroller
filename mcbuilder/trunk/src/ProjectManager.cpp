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


#include "ProjectManager.h"
#include <QTextStream>
#include <QtDebug>
#include <QDate>
#include <QFileInfo>
#include <QDir>
#include <QDomDocument>

/*
  ProjectManager is a class to handle various tasks related to
  creating and modifying mcbuilder projects.
*/

QString ProjectManager::createNewFile(const QString & projectPath, const QString & filePath)
{
  QFileInfo fi(filePath);
  if (fi.exists()) // if it already exists, don't do anything
    return "";

  QString newFileName;
  confirmValidFileName(&fi);
  QFile file(fi.filePath());

  if (file.exists()) // don't do anything if this file's already there
    return fi.filePath();
  if (file.open(QIODevice::WriteOnly | QFile::Text)) {
    QTextStream out(&file);
    out << QString("// %1").arg(fi.fileName()) << endl;
    out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl << endl;
    file.close();
    if (addToProjectFile(projectPath, fi.filePath()))
      newFileName = fi.filePath();
  }
  return newFileName;
}

/*
  Save an existing file with a new name.
*/
QString ProjectManager::saveFileAs(const QString & projectPath, const QString & existingFilePath, const QString & newFilePath)
{
  QFileInfo fi(newFilePath);
  if (fi.exists()) // if it already exists, don't do anything
    return fi.filePath();

  confirmValidFileName(&fi);
  QFile file(existingFilePath);
  if (!file.copy(fi.filePath()))
    return QString();

  if (addToProjectFile(projectPath, fi.filePath()))
    return fi.filePath();
  else
    return QString();
}

/*
  Only valid file suffixes are .c and .h
  If the suffix is missing or is not one of those, set it to .c by default
*/
void ProjectManager::confirmValidFileName(QFileInfo* fi)
{
  if (fi->baseName().contains(" ")) {
    QString newBaseName = fi->baseName().remove(" ");
    fi->setFile(fi->path() + "/" + newBaseName + "." + fi->suffix());
  }
  QStringList validSuffixes = QStringList() << "c" << "cpp" << "cxx" << "cc" << "h" << "hpp";
  if (!validSuffixes.contains(fi->suffix())) // default to a .c suffix if not provided
    fi->setFile(fi->path() + "/" + fi->baseName() + ".c");
}

/*
  Make sure a given project name is valid, and modify it if necessary.
  Spaces are not allowed since make is unhappy with file paths with spaces.
*/
QString ProjectManager::confirmValidProjectName(const QString & name)
{
  QString validname = name;
  if (name.contains(" ")) { // make sure the project name doesn't have any spaces
    QStringList elems = name.split(QDir::separator());
    elems.last().remove(" ");
    validname = elems.join(QDir::separator());
  }
  return validname;
}

/*
  Create a new project.
  - a new directory for the project
  - an XML project file for the project, from template
  - a stubbed out source file, from template
  Make sure the path name doesn't have any spaces in it, so make can work happily.
  Return the new project's name, or an empty string on failure.
*/
QString ProjectManager::createNewProject(const QString & newProjectPath)
{
  QString projectPath = confirmValidProjectName(newProjectPath);
  QDir newProjectDir;
  newProjectDir.mkpath(projectPath);
  newProjectDir.setPath(projectPath);
  QString newProjName = newProjectDir.dirName();

  // grab the templates for a new project
  QDir templatesDir("/Users/liam/Documents/mtcode/make/mcbuilder/resources/templates");

  // create the project file from our template
  QFile templateFile(templatesDir.filePath("project_template.xml"));
  templateFile.copy(newProjectDir.filePath(newProjName + ".xml"));
  templateFile.close();

  templateFile.setFileName(templatesDir.filePath("makefile_template.txt"));
  templateFile.copy(newProjectDir.filePath("Makefile"));
  templateFile.close();

  templateFile.setFileName(templatesDir.filePath("config_template.txt"));
  templateFile.copy(newProjectDir.filePath("config.h"));
  templateFile.close();

  templateFile.setFileName(templatesDir.filePath("source_template.txt"));
  if (templateFile.open(QIODevice::ReadOnly | QFile::Text)) {
    // and create the main file
    QFile mainFile(newProjectDir.filePath(newProjName + ".c"));
    if (mainFile.open(QIODevice::WriteOnly | QFile::Text)) {
      QTextStream out(&mainFile);
      out << QString("// %1.c").arg(newProjName) << endl;
      out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl;
      out << templateFile.readAll();
      mainFile.close();
    }
    QFileInfo fi(mainFile);
    addToProjectFile(newProjectDir.path(), fi.filePath());
    templateFile.close();
  }
  return newProjectDir.path();
}

/*
  Add a filepath to this project's file list.
  It's path should be relative to the project directory.
*/
bool ProjectManager::addToProjectFile(const QString & projectPath, const QString & newFilePath)
{
  bool retval = false;
  QDomDocument newProjectDoc;
  QDir projectDir(projectPath);
  QFile projectFile(projectDir.filePath(projectDir.dirName() + ".xml"));
  // read in the existing file, and add a node to the "files" section
  if (newProjectDoc.setContent(&projectFile)) {
    projectFile.close();
    QDomElement newFileElement = newProjectDoc.createElement("file");
    QDomText newFilePathElement = newProjectDoc.createTextNode(projectDir.relativeFilePath(newFilePath));
    newFileElement.appendChild(newFilePathElement);
    newProjectDoc.elementsByTagName("files").at(0).toElement().appendChild(newFileElement);

    // write our newly manipulated file
    if (projectFile.open(QIODevice::WriteOnly | QFile::Text)) { // reopen as WriteOnly
      projectFile.write(newProjectDoc.toByteArray(2));
      projectFile.close();
      retval = true;
    }
  }
  return retval;
}

/*
  Remove a file from the project file.
  Don't delete the file.
*/
bool ProjectManager::removeFromProjectFile(const QString & projectPath, const QString & filePath)
{
  bool retval = false;
  QDomDocument doc;
  QDir dir(projectPath);
  QFile projectFile(dir.filePath(dir.dirName() + ".xml"));
  if (doc.setContent(&projectFile)) {
    projectFile.close();
    QDomNodeList files = doc.elementsByTagName("files").at(0).childNodes();
    for (int i = 0; i < files.count(); i++) {
      if (files.at(i).toElement().text() == dir.relativeFilePath(filePath)) {
        QDomNode parent = files.at(i).parentNode();
        parent.removeChild(files.at(i));
        if (projectFile.open(QIODevice::WriteOnly|QFile::Text)) {
          projectFile.write(doc.toByteArray(2));
          retval = true;
        }
      }
    }
  }
  return retval;
}

/*
  Save a copy of a project.
  Any files in the project with the name of the project should be changed,
  others are simply copied over.
*/
QString ProjectManager::saveCurrentProjectAs(const QString & currentProjectPath, const QString & newProjectPath)
{
  QDir currentProjectDir(currentProjectPath);
  QString currentProjectName = currentProjectDir.dirName();
  QString newProjPath = confirmValidProjectName(newProjectPath);

  QDir newProjectDir;
  newProjectDir.mkpath(newProjPath);
  newProjectDir.setPath(newProjPath);
  QString newProjectName = newProjectDir.dirName();

  QFileInfoList fileList = currentProjectDir.entryInfoList();
  foreach (QFileInfo fi, fileList) {
    // give any project-specific files the new project's name
    if (fi.baseName() == currentProjectName) {
      if (fi.suffix() != "o") { // don't need to copy obj files
        QFile tocopy(fi.filePath());
        tocopy.copy(newProjectDir.filePath(newProjectName + "." + fi.suffix()));
      }
    }
    else { // just copy the file over
      QFile tocopy(fi.filePath());
      tocopy.copy(newProjectDir.filePath(fi.fileName()));
    }
  }

  // update the contents of the project file
  QDomDocument projectDoc;
  QFile projFile(newProjectDir.filePath(newProjectName + ".xml"));
  if(projectDoc.setContent(&projFile)) {
    projFile.close();
    QDomNodeList allFiles = projectDoc.elementsByTagName("files").at(0).childNodes();
    for(int i = 0; i < allFiles.count(); i++) {
      if(!allFiles.at(i).toElement().text().startsWith("resources/cores")) {
        QFileInfo fi(allFiles.at(i).toElement().text());
        if(fi.baseName() == currentProjectName) {
          fi.setFile(fi.path() + "/" + newProjectName + "." + fi.suffix());
          allFiles.at(i).firstChild().setNodeValue(QDir::cleanPath(fi.filePath()));
        }
      }
    }
    // reopen with WriteOnly
    if(projFile.open(QIODevice::WriteOnly|QFile::Text)) {
      projFile.write(projectDoc.toByteArray(2));
      projFile.close();
    }
  }
  return newProjectDir.path();
}
