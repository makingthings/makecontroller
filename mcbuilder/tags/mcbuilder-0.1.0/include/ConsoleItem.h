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


#ifndef CONSOLE_ITEM_H
#define CONSOLE_ITEM_H

#include <QListWidgetItem>

class ConsoleItem : public QListWidgetItem
{  
	public:
		enum Type { Error, Warning };
    ConsoleItem(QString filepath, int linenumber, Type type)
    {
      filepath_ = filepath;
      linenumber_ = linenumber;
      type_ = type;
    }
    QString filePath() { return filepath_; }
    int lineNumber() { return linenumber_; }
    Type messageType() { return type_; }
    
  private:
    QString filepath_;
    int linenumber_;
    Type type_;
};


#endif // CONSOLE_ITEM_H


