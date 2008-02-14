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
// OS X elements from code by Erik Gilling
/*
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 */

#ifndef UPLOADERTHREAD_H
#define UPLOADERTHREAD_H

//Qt includes
#include <QThread>

#include "McHelperWindow.h"
#include "Samba.h"
#include "SambaMonitor.h"

class McHelperWindow;
class Samba;

class UploaderThread : public QThread
{
	Q_OBJECT
		
	public:	
    UploaderThread( QApplication* application, McHelperWindow* mainWindow,
    					Samba* samba, SambaMonitor* monitor );
    ~UploaderThread( );
	  void run();
		void setBinFileName( char* filename );
		void setBootFromFlash( bool value );
		void showStatus( QString message, int duration );
		void progress( int value );
		QString getDeviceKey( );
		void setDeviceKey( QString key );
	  
	private:
	  QApplication* application;
	  McHelperWindow* mainWindow;
	  SambaMonitor* monitor;
		Samba* samba;
		QString deviceKey;
		
		char* bin_file;
		bool bootFromFlash;
};

class McHelperEvent : public QEvent
{
	public:
	  McHelperEvent( bool statusBound, int level, char* message );
	  ~McHelperEvent( );
	  
	bool statusBound;  
	int level;
	char* message;
};

class McHelperProgressEvent : public QEvent
{
	public:
	  McHelperProgressEvent( int progress );
	  ~McHelperProgressEvent( ) {}
	  
	int progress;
};

class StatusEvent : public QEvent
{
	public:
	  StatusEvent( QString message, int duration );
	  ~StatusEvent( ) {}
	  
	QString message;
	int duration;
};

#endif
