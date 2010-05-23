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


#include "BuildLog.h"
#include <QSettings>

/*
 Build log displays the raw output from GCC, if you want to have a look at it.
*/
BuildLog::BuildLog( ) : QDialog( )
{
  ui.setupUi(this);
  connect(ui.clearButton, SIGNAL(clicked()), ui.logConsole, SLOT(clear()));
  QSettings settings;
  QSize dialogSize = settings.value("build_log_size").toSize();
  if(dialogSize.isValid())
    resize(dialogSize);
}

/*
  Stuff some text into the log.
*/
void BuildLog::append(const QString & msg)
{
  if(msg.startsWith("*****************"))
    fmt.setForeground(Qt::black);
  else
    fmt.setForeground(QColor(75, 75, 75)); // gray

  ui.logConsole->textCursor().setBlockCharFormat(fmt);
  // for some reason, block coloring only seems to happen with new lines...shrug
  ui.logConsole->appendPlainText(msg + (!msg.endsWith("\n") ? "\n" : ""));

  // scroll to the bottom
  ui.logConsole->moveCursor(QTextCursor::End);
  ui.logConsole->ensureCursorVisible();
}

/*
  Clear the log.
*/
void BuildLog::clear()
{
  ui.logConsole->clear();
}






