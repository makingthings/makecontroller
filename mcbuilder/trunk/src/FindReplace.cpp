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


#include "FindReplace.h"

/*
  A simple find and replace dialog for the code editor.
*/
FindReplace::FindReplace( MainWindow *mw ) : QDialog( )
{
  setupUi(this);
  mainWindow = mw;
  connect( nextButton, SIGNAL(clicked()), this, SLOT(onNext()));
  connect( previousButton, SIGNAL(clicked()), this, SLOT(onPrevious()));
  connect( replaceButton, SIGNAL(clicked()), this, SLOT(onReplace()));
  connect( replaceAllButton, SIGNAL(clicked()), this, SLOT(onReplaceAll()));
  connect( findBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onNext()));
  connect( replaceBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onReplace()));
}

/*
  The "next" button has been clicked.
  Find the next instance of the text in the "find" box.
*/
void FindReplace::onNext( )
{
  if(!findBox->lineEdit()->text().isEmpty())
    mainWindow->findText(findBox->lineEdit()->text(), getFlags(true), true);
}

/*
  The "previous" button has been clicked.
  Find the previous instance of the text in the "find" box.
*/
void FindReplace::onPrevious( )
{
  if(!findBox->lineEdit()->text().isEmpty())
    mainWindow->findText(findBox->lineEdit()->text(), getFlags(false), false);
}

/*
  The "replace" button has been clicked.
  Replace the currently highlighted text with the text in the "replace" box.
*/
void FindReplace::onReplace( )
{
  mainWindow->replace(replaceBox->lineEdit()->text());
}

/*
  The "replace all" button has been clicked.
  Replace all instances of the text in the "find" box with the
  text in the "replace" box.
*/
void FindReplace::onReplaceAll( )
{
  mainWindow->replaceAll(findBox->lineEdit()->text(), replaceBox->lineEdit()->text(), getFlags());
}

/*
  Generate the appropriate flags for the find operation
  based on the current selections in the dialog.
*/
QTextDocument::FindFlags FindReplace::getFlags(bool forward)
{
  QTextDocument::FindFlags flags;
  if(!forward)
    flags |= QTextDocument::FindBackward;
  if(ignoreCaseCheckBox->checkState() != Qt::Checked)
    flags |= QTextDocument::FindCaseSensitively;
  if(wholeWordCheckBox->checkState() == Qt::Checked)
    flags |= QTextDocument::FindWholeWords;
  return flags;
}

