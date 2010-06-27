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

#include "TestProjectInfo.h"
#include <QCheckBox>

TestProjectInfo::TestProjectInfo(MainWindow* mw)
{
  mainWindow = mw;
  projectInfo = mw->projInfo;
}

/*
  Make sure that when a system is checked as included or not, the
  change is propagated to the xml file properly.
*/
void TestProjectInfo::includeSystem()
{
  QCheckBox* u = projectInfo->ui.usbBox;
  bool originalState = u->isChecked();
  QString projectPath = projectInfo->projectFilePath(mainWindow->currentProjectPath());
  projectInfo->load(projectPath);

  QFile file(projectPath);
  if (file.open(QIODevice::ReadOnly|QFile::Text)) {
    QDomDocument projectFile;
    if (projectFile.setContent(&file)) {
      QString originalString = (originalState) ? "true" : "false";
      QVERIFY(projectFile.elementsByTagName("include_usb").at(0).toElement().text() == originalString);
    }
    file.close();
  }
  else
    QFAIL("Couldn't open project file.");

//  QTest::mouseClick(u, Qt::LeftButton); // this doesn't seem to work anymore...
  u->setChecked(!originalState);
  QVERIFY(u->isChecked() != originalState);
  projectInfo->applyChanges();

  if (file.open(QIODevice::ReadOnly|QFile::Text)) {
    QDomDocument projectFile;
    if (projectFile.setContent(&file)) {
      QString newString = (originalState) ? "false" : "true";
      QVERIFY(projectFile.elementsByTagName("include_usb").at(0).toElement().text() == newString);
    }
    file.close();
  }
  else
    QFAIL("Couldn't open project file.");
}
