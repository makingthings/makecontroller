/*
  LED Web Server - MakingThings, 2008
  
  Control the application board LEDs through a web server interface.
  The default IP address of the Make Controller is 192.168.0.200, so once
  you've uploaded the project and plugged the board into your network, 
  type 192.168.0.200 into your browser and control the LEDs via the 
  checkboxes on the screen.
*/
#include "config.h"
#include "appled.h"
#include "webserver.h"

int LedHandler( char* requestType, char* address, char* requestBuffer, 
                      int requestMaxSize, void* socket, char* buffer, int len );

void Run( ) // this gets called as soon as we boot up.
{
  Network_SetActive(true); // turn on the network system
  WebServer_SetActive(true); // fire up our webserver
  WebServer_Route("/", LedHandler); // specify that we want to handle all requests with LedHandler
}

/*
  This is our handler.  It gets called by the webserver when an incoming request matches
  the route we supplied in WebServer_Route() above.
  We're going to write out a simple HTML form with some checkboxes, and if
  any of the checkboxes are selected, light up the corresponding LED.
*/
int LedHandler( char* requestType, char* address, char* requestBuffer, 
                      int requestMaxSize, void* socket, char* buffer, int len )
{
  (void)address; // unused parameter
  char temp[100];

  if( !WebServer_WriteResponseOkHTML( socket ) )
    return 0;

  if( !WebServer_WriteHeader( true, socket, buffer, len ) )
    return 0;

  if( !WebServer_WriteBodyStart( 0, socket, buffer, len ) )
    return 0;
  
  int formElements = 0;
  HtmlForm form;
  form.count = 0;
  
  // extract the form data, depending on how the form was submitted - either GET or POST
  if ( strncmp( requestType, "GET", 3 ) == 0 )
  {
    char *p = strchr( requestBuffer, '?' );
    if( p != NULL ) // if we didn't find a ?, then there were no form elements
      formElements = WebServer_ParseFormElements( p+1, &form );
  }
  else if ( strncmp( requestType, "POST", 4 ) == 0 )
  {
    // make sure we're pointing at the POST data and if it looks good, process it
    if( WebServer_GetPostData( socket, requestBuffer, requestMaxSize ) )
      formElements = WebServer_ParseFormElements( requestBuffer, &form ); // grab the data out of the form
  }
  
  // write out a form with checkboxes for each LED.
  strcat( buffer, "<h1>Welcome!  Have some fun with the LEDs, please.</h1>" );
  strcat( buffer, "<form method=\"POST\">" );
  strcat( buffer, "App LED 0: <input type=\"checkbox\" name=\"appled0\"><br>" );
  strcat( buffer, "App LED 1: <input type=\"checkbox\" name=\"appled1\"><br>" );
  strcat( buffer, "App LED 2: <input type=\"checkbox\" name=\"appled2\"><br>" );
  strcat( buffer, "App LED 3: <input type=\"checkbox\" name=\"appled3\"><br>" );
  strcat( buffer, "<p></p>" ); 
  strcat( buffer, "<input type=\"submit\" value=\"Submit\">" );
  strcat( buffer, "</form>" );
  
  // now deal with any form elements we may have parsed from above
  int i, j;
  for( j = 0; j < 4; j++ )
  {
    int value = 0;
    snprintf( temp, 100, "appled%d", j ); // for each LED, see if we got a form element with its name
    for( i = 0; i < formElements; i++ )
    {
      if( strcmp( temp, form.elements[i].key ) == 0 ) // check the name of the key
        value = 1; // if the checkbox was checked, light up the LED
    }
    AppLed_SetState( j, value ); 
  }
  
  // Write out the dynamically generated page.
  if( !SocketWrite( socket, buffer, strlen( buffer ) ) )
    return 0 ;
  
  WebServer_WriteBodyEnd( socket );
  return 1;
}


