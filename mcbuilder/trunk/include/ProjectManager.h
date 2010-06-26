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

#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <QString>
#include <QFileInfo>

class ProjectManager
{
  public:
    ProjectManager( ) { }
    QString createNewFile(const QString & projectPath, const QString & filePath);
    QString saveFileAs(const QString & projectPath, const QString & existingFilePath, const QString & newFilePath);
    bool addToProjectFile(const QString & projectPath, const QString & newFilePath);
    bool removeFromProjectFile(const QString & projectPath, const QString & filePath);
    QString createNewProject(const QString & newProjectPath);
    QString saveCurrentProjectAs(const QString & currentProjectPath, const QString & newProjectPath);

  private:
    void confirmValidFileName(QFileInfo* fi);
    QString confirmValidProjectName(const QString & name);
};

#endif // PROJECT_MANAGER_H

