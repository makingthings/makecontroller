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


#ifndef SAMBA_MONITOR_H_
#define SAMBA_MONITOR_H_

#include <QList>
#include <QHash>
#include <QThread>
#include "Samba.h"
#include "UploaderThread.h"
#include "McHelperWindow.h"

class Samba;
class McHelperWindow;
class UploaderThread;

class SambaMonitor : public QThread
{
  public:
  	SambaMonitor( QApplication* application, McHelperWindow* mainWindow );
  	~SambaMonitor( );
  	void run( );
  	int scan( QList<UploaderThread*>* arrived );
  	void deviceRemoved( QString key );
  	bool alreadyHas( QString key );
  	void closeAll( );
  	
  private:
  	QHash<QString, UploaderThread*> connectedDevices; // our internal list
  	// right after an upload, we don't want the board to show up in our list
  	// but it will still show up as being attached to the system, so we need another list
  	QList<QString> awaitingRemoval; 
	
	MessageInterface* messageInterface;
	QApplication* application;
	McHelperWindow* mainWindow;
};

#endif // SAMBA_MONITOR_H_











