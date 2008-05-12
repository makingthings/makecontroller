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

#include "MessageEvent.h"

MessageEvent::MessageEvent( QString string, MessageEvent::Types type, QString from ) : QEvent( (Type)10003 )
{
  this->message = string;
  this->type = type;
  this->from = from;
}

MessageEvent::MessageEvent( QStringList strings, MessageEvent::Types type, QString from ) : QEvent( (Type)10004 )
{
  this->messages = strings;
  this->type = type;
  this->from = from;
}

