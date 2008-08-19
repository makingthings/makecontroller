/*
  HTTP Get - MakingThings, 2008
  
  Connect to a website, read some info from it, and print it out
  to the USB Console.
*/
#include "config.h"
#include "webclient.h"

void HttpGetTask( void* p );
void Blink(void* p);

void Run( ) // this task gets called as soon as we boot up.
{
  Usb_SetActive(true); // turn on USB
  TaskCreate( Blink, "Blink", 1000, 0, 1 );
  Network_SetActive(true); // turn on the network
  TaskCreate( HttpGetTask, "GET", 2000, 0, 3 );
}

#define HTTP_PORT 80
#define READ_BUF_LEN 1024
char readBuffer[READ_BUF_LEN];

void HttpGetTask( void* p )
{
  (void)p; // unused
  char* hostname = "www.wikipedia.org"; // change this to whatever website you'd like to connect to
  /*
    Note that some websites tend to refuse connections, possibly because we
    don't supply as much info to the server as a browser does, for example.
    Most simple sites should work fine, however.
  */

  while( true )
  {
    int address = Network_DnsGetHostByName(hostname);
    // if you'd like to connect to a certain page on the site, change the "/" to the desired address.
    // ie, "/page/I/want/to/read"
    int getSize = WebClient_Get( address, HTTP_PORT, hostname, "/", readBuffer, READ_BUF_LEN );
    if( getSize > 0 ) // if we read anything, write it back out to the Console
    {
      Usb_Write(readBuffer, getSize);
      Usb_Write("\n\n", 2); // a couple newlines for formatting
    }
    else
      Usb_Write("Connect error.\n", 15);
    Sleep(15000); // do this once every 15 seconds
  }
}

void Blink(void* p)
{
  (void)p;
  Led_SetState(1);
  Sleep(1000);
  while(1)
  {
    Led_SetState(0);
    Sleep(990);
    Led_SetState(1);
    Sleep(10);
  }
}

