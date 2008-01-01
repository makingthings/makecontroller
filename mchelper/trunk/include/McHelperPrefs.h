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

#ifndef MCHELPERPREFS_H
#define MCHELPERPREFS_H

#include "ui_mchelperPrefs.h"
#include "McHelperWindow.h"

class McHelperWindow;

class mchelperPrefs : public QDialog, public Ui::mchelperPrefs
{
	Q_OBJECT
	public:
		mchelperPrefs( McHelperWindow *mainWindow );
		void setUdpListenPortDisplay( int port );
		void setUdpSendPortDisplay( int port );
		void setXmlPortDisplay( int port );
		void setMaxMsgsDisplay( int max );
		void setFindNetBoardsDisplay( bool state );
		bool getFindNetBoardsDisplay( );
		bool getUpdateCheckBoxDisplay( );
			
	private:
		McHelperWindow *mainWindow;
};


#endif // MCHELPERPREFS_H
