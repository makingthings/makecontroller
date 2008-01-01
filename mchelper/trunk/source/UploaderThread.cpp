/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/
// OS X elements from code by Erik Gilling
/*
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 */

#include "UploaderThread.h"

UploaderThread::UploaderThread( QApplication* application, McHelperWindow* mainWindow, 
								Samba* samba, SambaMonitor* monitor ) : QThread()
{
  this->application = application;
  this->mainWindow = mainWindow;
	this->samba = samba;
	this->monitor = monitor;
}

UploaderThread::~UploaderThread( )
{
	if( samba != NULL )
		delete samba;
}


void UploaderThread::run()
{
	if ( samba->connect( deviceKey ) != Samba::OK )
	{
		mainWindow->messageThreadSafe( QString( "Usb> Upload Failed - couldn't connect.") );
		mainWindow->messageThreadSafe( QString( "  ** Check USB cable is plugged in.") );
		mainWindow->messageThreadSafe( QString( "  ** Make sure you've erased the current program, reset the power, and try again.") );
		Samba::Status disconnectStatus;
		disconnectStatus = samba->disconnect( );
		return;
	}
	
	Samba::Status uploaderStatus = samba->flashUpload( bin_file );
	if ( uploaderStatus != Samba::OK )
  {
  	showStatus( QString( "Usb> Upload Failed." ), 2000 );
  	switch ( uploaderStatus )
  	{
  		case Samba::ERROR_INCORRECT_CHIP_INFO:
			mainWindow->messageThreadSafe( QString( 
				"Usb> Upload Failed - don't recognize the chip you're trying to program.") );
			  break;
  		case Samba::ERROR_COULDNT_FIND_FILE:
  		case Samba::ERROR_COULDNT_OPEN_FILE:
			mainWindow->messageThreadSafe( QString( 
				"Usb> Upload Failed - Couldn't find or open the specified .bin file.") );
			  break;
  		case Samba::ERROR_SENDING_FILE:
			mainWindow->messageThreadSafe( QString( 
				"Usb> Upload Failed - Couldn't complete download.") );
    		mainWindow->messageThreadSafe( QString( 
    			"  ** Noisy power supply?  Flakey USB connection?") );
			  break;
  		default:
			mainWindow->messageThreadSafe( QString( 
				"Usb> Upload Failed - Unknown Error - %d.").arg(uploaderStatus) );
    		mainWindow->messageThreadSafe( QString( 
    			"  ** Note error number and consult the support forums.") );
			  break;	
  	}
  }
	
	// set up samba to actually boot from this file
	if ( samba->bootFromFlash( ) != Samba::OK )
		mainWindow->messageThreadSafe( QString( "Usb> Could not switch to boot from flash.") );
		
	if ( samba->reset(  ) != Samba::OK )
		mainWindow->messageThreadSafe( QString( "Usb> Could not switch to boot from flash.") );
		
	showStatus( QString( "Upload complete." ), 2000 );
	progress( -1 );
	monitor->deviceRemoved( deviceKey );
}

QString UploaderThread::getDeviceKey( )
{
	return deviceKey;
}

void UploaderThread::setDeviceKey( QString key )
{
	this->deviceKey = key;
}

void UploaderThread::setBinFileName( char* filename )
{
	bin_file = filename;
}

void UploaderThread::setBootFromFlash( bool value )
{
	bootFromFlash = value;
}

void UploaderThread::showStatus( QString message, int duration )
{
	StatusEvent* statusEvent = new StatusEvent( message, duration );
	application->postEvent( mainWindow, statusEvent );
}

void UploaderThread::progress( int value )
{
	McHelperProgressEvent* mcHelperProgressEvent = new McHelperProgressEvent( value );
	application->postEvent( mainWindow, mcHelperProgressEvent );
}

McHelperEvent::McHelperEvent( bool statusBound, int level, char* message ) : QEvent( (Type)10000 )
{
	this->statusBound = statusBound;
	this->level = level;
	
  if ( message != 0 )
	  this->message = strdup( message ); 
	else
	  this->message = 0;
}

McHelperEvent::~McHelperEvent( )
{
  if ( message != 0 )
    free( message );
}

McHelperProgressEvent::McHelperProgressEvent( int progress ) : QEvent( (Type)10001 )
{
	this->progress = progress; 
}

StatusEvent::StatusEvent( QString message, int duration ) : QEvent( (Type)10010 )
{
	this->message = message;
	this->duration = duration;
}



