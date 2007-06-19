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
// OS X elements from code by Erik Gilling
/*
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 */

#include "UploaderThread.h"

UploaderThread::UploaderThread( QApplication* application, McHelperWindow* mainWindow, Samba* samba ) : QThread()
{
  this->application = application;
  this->mainWindow = mainWindow;
	this->samba = samba;
}

UploaderThread::~UploaderThread( )
{
	if( samba != NULL )
	{
		samba->disconnect( );
		delete samba;
	}
}


void UploaderThread::run()
{
	if ( samba->connect( ) != Samba::OK )
	{
		message( 1, "usb> Upload Failed - couldn't connect.\n" );
		message( 1, "  ** Check USB cable is plugged in.\n" );
		message( 1, "  ** Make sure you've erased the current program, reset the power, and try again.\n" );
		Samba::Status disconnectStatus;
		disconnectStatus = samba->disconnect( );
		return;
	}
	
	Samba::Status uploaderStatus = samba->flashUpload( bin_file );
	if ( uploaderStatus != Samba::OK )
  {
  	switch ( uploaderStatus )
  	{
  		case Samba::ERROR_INCORRECT_CHIP_INFO:
				message( 1, "usb> Upload Failed - don't recognize the chip you're trying to program.\n" );
			  break;
  		case Samba::ERROR_COULDNT_FIND_FILE:
  		case Samba::ERROR_COULDNT_OPEN_FILE:
				message( 1, "usb> Upload Failed - Couldn't find or open the specified .bin file.\n" );
			  break;
  		case Samba::ERROR_SENDING_FILE:
				message( 1, "usb> Upload Failed - Couldn't complete download.\n" );
    		message( 1, "  ** Noisy power supply?  Flakey USB connection?\n");
			  break;
  		default:
				message( 1, "usb> Upload Failed - Unknown Error - %d\n", uploaderStatus );
    		message( 1, "  ** Note error number and consult the support forums.\n");
			  break;	
  	}
  }
	
	if( bootFromFlash )
	{
		if ( samba->bootFromFlash(  ) != Samba::OK )
			message( 1, "usb> Could not switch to boot from flash.\n" );
		bootFromFlash = false;
	}
	
	if ( samba->disconnect(  ) != Samba::OK )
  	message( 1, "usb> Error disconnecting.\n" );
		
	message( 1, "usb> Upload complete - reset the power on the board to run the new program.\n" );
	progress( -1 );
}

QString UploaderThread::getDeviceKey( )
{
	return samba->getDeviceKey( );
}

void UploaderThread::setBinFileName( char* filename )
{
	bin_file = filename;
}

void UploaderThread::setBootFromFlash( bool value )
{
	bootFromFlash = value;
}

void UploaderThread::message( int level, char *format, ... )
{
	va_list args;
	char buffer[ 1000 ];
	
	va_start( args, format );
	vsnprintf( buffer, 1000, format, args );
	va_end( args );
	
	McHelperEvent* mcHelperEvent = new McHelperEvent( false, level, buffer );
	
	if( level == 1 )
	  application->postEvent( mainWindow, mcHelperEvent );
}

void UploaderThread::progress( int value )
{
	McHelperProgressEvent* mcHelperProgressEvent = new McHelperProgressEvent( value );
	application->postEvent( mainWindow, mcHelperProgressEvent );
}

void UploaderThread::sleepMs( int ms )
{
  //usleep( ms * 1000 );	
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

McHelperProgressEvent::~McHelperProgressEvent( )
{

}


