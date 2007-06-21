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


#include "McHelperWindow.h"

#include <QApplication>
#include <QMessageBox> 

#ifdef Q_WS_WIN
#include "dbt.h"
#define DBT_DEVICEREMOVECOMPLETE 0x8004

bool McHelperApp::winEventFilter( MSG* msg, long* retVal )
{	
	if ( msg->message == WM_DEVICECHANGE )
	{
		if( msg->wParam == DBT_DEVICEREMOVECOMPLETE )
		{
			PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
			if( lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE )
			{
				PDEV_BROADCAST_HANDLE lpdbv = (PDEV_BROADCAST_HANDLE)lpdb;
				mchelper->usbRemoved( lpdbv->dbch_handle );  // call back to get the usb port shut down.
				*retVal = false;
				return true;
			}
		}
	}
	return false;
}

void McHelperApp::setMainWindow( McHelperWindow* window )
{
	mchelper = window;
}
#endif // Windows-only

int main(int argc, char *argv[])
{
	McHelperApp app(argc, argv);
	
	McHelperWindow mcHelperWindow( &app );
	
	if( argc < 2 )
	{
		QDialog *aboutMchelper = new QDialog( );
        Ui::aboutMchelper ui;
        ui.setupUi(aboutMchelper);
        mcHelperWindow.setAboutDialog( aboutMchelper );
        
		mcHelperWindow.show();
		mcHelperWindow.setNoUI( false );
	} else
	{	
		char* argv1 = argv[1];
		mcHelperWindow.setNoUI( true );
		mcHelperWindow.uiLessUpload( argv1, true );
	}
	return app.exec();
}

