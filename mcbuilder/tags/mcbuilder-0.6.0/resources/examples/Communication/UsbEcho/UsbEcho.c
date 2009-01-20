/* 
  UsbEcho - MakingThings, 2008

  Listen on the USB port, and echo back any info that we receive.
  Build and upload this, then open the USB console.
  Anything you send the board should be echoed back to you.
*/
#include "config.h"

void UsbEchoTask( void* p );

void Run( ) // this task gets called as soon as we boot up.
{
  Usb_SetActive(1); // turn on the USB system
  TaskCreate( UsbEchoTask, "UsbEcho", 1000, 0, 3 );
}

#define USB_BUFF_SIZE 256

void UsbEchoTask( void* p )
{
  (void)p;
  char buf[USB_BUFF_SIZE]; // a buffer for any incoming USB data

  while( true )
  {
    // try to read from the USB port
    int read = Usb_Read(buf, USB_BUFF_SIZE);
    // if we read anything, echo it back
    if(read)
      Usb_Write(buf, read);
    Sleep(1);
  }
}


