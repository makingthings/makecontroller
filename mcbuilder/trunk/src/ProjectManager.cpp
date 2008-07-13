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


#include "ProjectManager.h"
#include <QTextStream>
#include <QDate>
#include <QFileInfo>
#include <QDir>
#include <QDomDocument>

/*
  ProjectManager is a class to handle various tasks related to
  creating and modifying mcbuilder projects.
*/

bool ProjectManager::createNewFile(QString projectPath, QString filePath)
{
  QFileInfo fi(filePath);
  if(fi.suffix().isEmpty())
    fi.setFile(fi.filePath() + ".c");
  QFile file(fi.filePath());
  
  bool retval = false;
    
  if(file.exists()) // don't do anything if this file's already there
    return retval;
  if(file.open(QIODevice::WriteOnly | QFile::Text))
  {
    QTextStream out(&file);
    out << QString("// %1").arg(fi.fileName()) << endl;
    out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl << endl;
    file.close();
    addToProjectFile(projectPath, fi.filePath(), "thumb");
    retval = true;
  }
  return retval;
}

/*
  Save an existing file with a new name.
*/
bool ProjectManager::saveFileAs(QString projectPath, QString existingFilePath, QString newFilePath)
{
  QFileInfo fi(newFilePath);
  if(fi.exists()) // if it already exists, don't do anything
    return true;
  
  if(fi.suffix() != "c" && fi.suffix() != "h") // default to a .c suffix if not provided
    fi.setFile(fi.path() + "/" + fi.baseName() + ".c");

  QFile file(existingFilePath);
	if(!file.copy(fi.filePath()))
    return false;
  	
  if(addToProjectFile(projectPath, fi.filePath(), "thumb"))
    return true;
  else
    return false;
}

/*
  Create a new project.
  - a new directory for the project
  - an XML project file for the project, from template
  - a stubbed out source file, from template
  Make sure the path name doesn't have any spaces in it, so make can work happily.
  Return the new project's name, or an empty string on failure.
*/
QString ProjectManager::createNewProject(QString newProjectPath)
{
  if(newProjectPath.contains(" ")) // make sure the project name doesn't have any spaces
  {
    QStringList elems = newProjectPath.split(QDir::separator());
    elems.last().remove(" ");
    newProjectPath = elems.join(QDir::separator());
  }
  if(newProjectPath.contains(" ")) // if there are still spaces in the path, we have problems
    return "";
  QDir newProjectDir;
  newProjectDir.mkpath(newProjectPath);
  newProjectDir.setPath(newProjectPath);
  QString newProjName = newProjectDir.dirName();
  
  // grab the templates for a new project
  QDir templatesDir = QDir::current().filePath("resources/templates");
    
  // create the project file from our template
  QFile templateFile(templatesDir.filePath("project_template.xml"));
  templateFile.copy(newProjectDir.filePath(newProjName + ".xml"));
  templateFile.close();
  
  templateFile.setFileName(templatesDir.filePath("source_template.txt"));
  if( templateFile.open(QIODevice::ReadOnly | QFile::Text) )
  {
    // and create the main file
    QFile mainFile(newProjectDir.filePath(newProjName + ".c"));
    if( mainFile.open(QIODevice::WriteOnly | QFile::Text) )
    {
      QTextStream out(&mainFile);
      out << QString("// %1.c").arg(newProjName) << endl;
      out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl;
      out << templateFile.readAll();
      mainFile.close();
    }
    QFileInfo fi(mainFile);
    addToProjectFile(newProjectDir.path(), fi.filePath(), "thumb");
    templateFile.close();
  }
  return newProjectDir.path();
}

/*
  Add a filepath to this project's file list.
  It's path should be relative to the project directory.
*/
bool ProjectManager::addToProjectFile(QString projectPath, QString newFilePath, QString buildtype)
{
  bool retval = false;
  QDomDocument newProjectDoc;
  QDir projectDir(projectPath);
  QFile projectFile(projectDir.filePath(projectDir.dirName() + ".xml"));
  // read in the existing file, and add a node to the "files" section
  if(newProjectDoc.setContent(&projectFile))
  {
    projectFile.close();
    QDomElement newFileElement = newProjectDoc.createElement("file");
    newFileElement.setAttribute("type", buildtype);
    QDomText newFilePathElement = newProjectDoc.createTextNode(projectDir.relativeFilePath(newFilePath));
    newFileElement.appendChild(newFilePathElement);
    newProjectDoc.elementsByTagName("files").at(0).toElement().appendChild(newFileElement);
    
    // write our newly manipulated file
    if(projectFile.open(QIODevice::WriteOnly | QFile::Text)) // reopen as WriteOnly
    {
      projectFile.write(newProjectDoc.toByteArray(2));
      projectFile.close();
      retval = true;
    }
  }
  return retval;
}

//bool ProjectManager::removeFromProjectFile(QString projectPath, QString filePath)
//{
//  bool retval = false;
//  return retval;
//}

/*
  Save a copy of a project.
  Any files in the project with the name of the project should be changed,
  others are simply copied over.
*/
QString ProjectManager::saveCurrentProjectAs(QString currentProjectPath, QString newProjectPath)
{
  QDir currentProjectDir(currentProjectPath);
  QString currentProjectName = currentProjectDir.dirName();
  if(newProjectPath.contains(" ")) // make sure the project name doesn't have any spaces
  {
    QStringList elems = newProjectPath.split(QDir::separator());
    elems.last().remove(" ");
    newProjectPath = elems.join(QDir::separator());
  }
  if(newProjectPath.contains(" ")) // if there are still spaces elsewhere in the path, we have problems
    return "";
    
  QDir newProjectDir;
  newProjectDir.mkpath(newProjectPath);
  newProjectDir.setPath(newProjectPath);
  QString newProjectName = newProjectDir.dirName();
  
  QFileInfoList fileList = currentProjectDir.entryInfoList();
  foreach(QFileInfo fi, fileList)
  {
    // give any project-specific files the new project's name
    if(fi.baseName() == currentProjectName)
    {
      if(fi.suffix() != "o") // don't need to copy obj files
      {
        QFile tocopy(fi.filePath());
        tocopy.copy(newProjectDir.filePath(newProjectName + "." + fi.suffix()));
      }
    }
    else // just copy the file over
    {
      QFile tocopy(fi.filePath());
      tocopy.copy(newProjectDir.filePath(fi.fileName()));
    }
  }
  
  // update the contents of the project file
  QDomDocument projectDoc;
  QFile projFile(newProjectDir.filePath(newProjectName + ".xml"));
  if(projectDoc.setContent(&projFile))
  {
    projFile.close();
    QDomNodeList allFiles = projectDoc.elementsByTagName("files").at(0).childNodes();
    for(int i = 0; i < allFiles.count(); i++)
    {
      if(!allFiles.at(i).toElement().text().startsWith("resources/cores"))
      {
        QFileInfo fi(allFiles.at(i).toElement().text());
        if(fi.baseName() == currentProjectName)
        {
          fi.setFile(fi.path() + "/" + newProjectName + "." + fi.suffix());
          allFiles.at(i).firstChild().setNodeValue(QDir::cleanPath(fi.filePath()));
        }
      }
    }
    // reopen with WriteOnly
    if(projFile.open(QIODevice::WriteOnly|QFile::Text))
    {
      projFile.write(projectDoc.toByteArray(2));
      projFile.close();
    }
  }
  return newProjectDir.path();
}






