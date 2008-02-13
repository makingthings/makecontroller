/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef MESSAGE_EVENT_H_
#define MESSAGE_EVENT_H_

#include <QString>
#include <QEvent>
#include <QStringList>

class MessageEvent : public QEvent
{
  public:
    enum Types{ Warning, Error, Info, Notice, Response, Command, XMLMessage };
    
    MessageEvent( QString string, MessageEvent::Types type, QString from );
    ~MessageEvent( ) {}
    MessageEvent( QStringList messages, MessageEvent::Types type, QString from );
    
    QString message;
    QString from;
    MessageEvent::Types type;
    QStringList messages;
};

#endif /*MESSAGE_EVENT_H_*/
