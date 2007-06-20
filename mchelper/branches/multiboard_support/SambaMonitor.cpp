/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "SambaMonitor.h"

SambaMonitor::SambaMonitor( QApplication* application, McHelperWindow* mainWindow, BoardListModel* boardModel )
{
	this->application = application;
	this->mainWindow = mainWindow;
	this->boardModel = boardModel;
}

SambaMonitor::~SambaMonitor( )
{
	
}

int SambaMonitor::scan( QList<UploaderThread*>* arrived )
{
	int found = 0;
	QList<QString> sambaDevices;
	Samba* tempSamba = new Samba( this, mainWindow );
	tempSamba->FindUsbDevices( &sambaDevices );
	delete tempSamba;

	int i;
	for( i=0; i < sambaDevices.size( ); i++ )
	{
		if( !connectedDevices.contains( sambaDevices.at(i) ) && !awaitingRemoval.contains( sambaDevices.at(i) ) )
		{
			Samba* samba = new Samba( this, mainWindow );
			UploaderThread* uploaderThread = new UploaderThread( application, mainWindow, samba, this );
			samba->setUploader( uploaderThread );
			uploaderThread->setDeviceKey( sambaDevices.at(i) );
			arrived->append( uploaderThread );
			connectedDevices.insert( sambaDevices.at(i), uploaderThread );
			found++;
		}
	}

	if( sambaDevices.size( ) < connectedDevices.size( ) )
	{
		QHash<QString, UploaderThread*>::iterator i = connectedDevices.begin( );
		while( i != connectedDevices.end( ) )
		{
			if( !sambaDevices.contains( i.key() ) ) // then it must have gone away
			{
				if( awaitingRemoval.contains( i.key( ) ) )
					awaitingRemoval.removeAt( awaitingRemoval.indexOf( i.key( ) ) );
				else
					boardModel->removeBoard( i.key( ) );
				//delete i.value( );
				
				i = connectedDevices.erase( i ); // this increments the iterator
			}
			else
				++i;
		}
	}
	
	return found;
}

bool SambaMonitor::alreadyHas( QString key )
{
	return connectedDevices.contains( key );
}

void SambaMonitor::closeAll( )
{
	QHash<QString, UploaderThread*>::iterator i = connectedDevices.begin( );
	while( i != connectedDevices.end( ) )
	{
		delete i.value( );
		i = connectedDevices.erase( i ); // this increments the iterator
	}
}

void SambaMonitor::deviceRemoved( QString key )
{
	awaitingRemoval.append( key );
	boardModel->removeBoardThreadSafe( key );
}










