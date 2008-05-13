
#include "UsbMonitor.h"

UsbMonitor::UsbMonitor( ) : QDialog( )
{
	setupUi(this);
  connect( sendButton, SIGNAL(clicked()), this, SLOT(onCommandLine()));
  //connect( commandLine->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onCommandLine()));
  connect( viewList, SIGNAL(activated(QString)), this, SLOT(onView(QString)));
  connect( portList, SIGNAL(activated(QString)), this, SLOT(onPort(QString)));
}

/* 
 Scan the available ports & populate the list.
 If one of the ports is the one that was last open, open it up.
 Otherwise, wait for the user to select one then open that one.
*/
bool UsbMonitor::loadAndShow( )
{
  this->show();
	return true;
}

/*
 Send the contents of the commandLine to the serial port,
 and add them to the output console
*/
void UsbMonitor::onCommandLine( )
{
  
}

/*
 If the view has changed, update the contents of the 
 output console accordingly.
*/
void UsbMonitor::onView(QString view)
{
  (void)view;
}

/*
 The user has selected a port. 
 Close any open connections and open the new one.
*/
void UsbMonitor::onPort(QString port)
{
  (void)port;
}





