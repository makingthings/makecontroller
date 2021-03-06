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

#include "ext.h"
#include "ext_common.h"
#include "ext_obex.h"

#include "mc_osc.h"
#include "mcError.h"
#include "usb_serial.h"
#include "usb_enum.h"

#define MAXSIZE 512
#define MAX_READ_LENGTH 16384

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

// Max object structure def.
typedef struct _mcUsb
{
  t_object mcUsb_ob;
  t_symbol *symval;
  //Max things
  void* mc_clock;
  void* usb_clock;
  void *out0;
  //OSC things
  t_osc_message* osc_message; // the message to be sent out into Max
  t_osc* Osc;  //the osc object
  bool incomingPacketStarted;
  // USB things
  t_usbInterface* mc_usbInt;  // the USB interface
  char* usbReadBufPtr;
  char usbReadBuffer[ MAX_READ_LENGTH ];
	int usbReadBufLength;
  bool debug;
} t_mcUsb;

t_class* mcUsb_class;  // global variable pointing to the class

void *mcUsb_new( t_symbol *s, long ac, t_atom *av );
void mcUsb_free(t_mcUsb *x);
void mcUsb_anything(t_mcUsb *x, t_symbol *s, short ac, t_atom *av);
void mcUsb_assist(t_mcUsb *x, void *b, long m, long a, char *s);
void mcUsb_tick( t_mcUsb *x );
void usb_tick( t_mcUsb *x );
mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length );
void mc_SLIP_receive( t_mcUsb *x );
int mc_getMoreBytes( t_mcUsb *x );
void mcUsb_devicepath( t_mcUsb *x );
void mcUsb_debug( t_mcUsb *x, t_symbol *s, short ac, t_atom *av );
static int mcusb_obj_count;

// define the object so Max knows all about it, and define which messages it will implement and respond to
int main( )
{  	
	mcUsb_class = class_new("mc.usb", (method)mcUsb_new, (method)mcUsb_free, (short)sizeof(t_mcUsb), 0L, A_GIMME, 0);

	class_addmethod(mcUsb_class, (method)mcUsb_anything, "anything", A_GIMME, 0);
	class_addmethod(mcUsb_class, (method)mcUsb_assist, "assist", A_CANT, 0);
	class_addmethod(mcUsb_class, (method)mcUsb_devicepath, "devicepath", A_GIMME, 0);
  class_addmethod(mcUsb_class, (method)mcUsb_debug, "debug", A_GIMME, 0);

	// we want this class to instantiate inside of the Max UI; ergo CLASS_BOX
	class_register(CLASS_BOX, mcUsb_class);
  mcusb_obj_count = 0;

	return 0;
}

// gets called by Max when the object receives a "message" in its left inlet
void mcUsb_anything(t_mcUsb *x, t_symbol *s, short ac, t_atom *av)
{
	if (ac > MAXSIZE)
		ac = MAXSIZE;
	
	if( Osc_create_message( x->Osc, s->s_name, ac, av ) == MC_OK )
		mc_send_packet( x, x->mc_usbInt, x->Osc->outBuffer, (OSC_MAX_MESSAGE - x->Osc->outBufferRemaining) );
}

// set the inlet and outlet assist messages when somebody hovers over them in an unlocked patcher
void mcUsb_assist(t_mcUsb *x, void *b, long msg, long arg, char *s)
{
	if ( msg == ASSIST_INLET )
	{
		switch (arg)
		{
			case 0: sprintf(s,"Outgoing data - OSC messages");
				break;
		}
	}
	else if( msg == ASSIST_OUTLET ) 
	{
		switch (arg)
		{
			case 0: sprintf(s,"Incoming data - OSC messages"); 
				break;
		}
	}
}

// function that gets called back by the clock.
void mcUsb_tick( t_mcUsb *x )
{	
	if( x->mc_usbInt->deviceOpen )
  {
	  mc_SLIP_receive( x );
    clock_delay( x->mc_clock, 1 ); //set the delay interval for the next tick
  }
	else
	  clock_delay( x->mc_clock, 100 );
}

void usb_tick( t_mcUsb *x )
{
  bool success = findUsbDevice( x->mc_usbInt, FIND_MAKE_CONTROLLER );
  
  if( success && !x->mc_usbInt->deviceOpen ) // we found a new one, but we're not currently open
  {
    
    usb_open( x->mc_usbInt, FIND_MAKE_CONTROLLER );
    if( !x->mc_usbInt->deviceOpen ) // if we're still not open, bail
      return;
    post( "mc.usb connected to a Make Controller." );        
  }
  else if( !success && x->mc_usbInt->deviceOpen ) // we're currently open but can no longer find the device
  {
    usb_close( x->mc_usbInt );
    post( "mc.usb closed the Make Controller Kit USB connection." );
  }
  clock_delay( x->usb_clock, 250 );
}

int mc_getMoreBytes( t_mcUsb *x )
{
  int retval = 0;
  if( x->usbReadBufLength < 1 ) // if we don't have any unprocessed chars in our buffer, go read some more
	{
		int available = usb_numBytesAvailable( x->mc_usbInt );
    if( available < 0 )
			return -1;
		if( available > 0 )
		{
			int justGot;
      if( available > MAX_READ_LENGTH )
				available = MAX_READ_LENGTH;
			justGot = usb_read( x->mc_usbInt, x->usbReadBuffer, available );
      //post("read: avail %d, got %d", available, justGot);
      if( justGot < 0 )
        return -1;
    	x->usbReadBufPtr = x->usbReadBuffer;
			x->usbReadBufLength += justGot;
		}
	}
  return retval;
}

void mc_SLIP_receive( t_mcUsb *x )
{
  int finished = 0;
  t_osc* osc = x->Osc;
	
	while( true )
	{
    if( !x->usbReadBufLength )
    {
      if( mc_getMoreBytes( x ) < 0 )
        return;
      if( !x->usbReadBufLength )
        return;
    }
    
    while( x->usbReadBufLength )
		{
      //post("rx: %c, len %d", *x->usbReadBufPtr, osc->inbuf_length);
      switch( (unsigned char)*x->usbReadBufPtr )
			{
				case END:
					if( x->incomingPacketStarted && osc->inbuf_length > 0 ) // it was the END byte
					{
						Osc_receive_packet( x->out0, osc, osc->inBuffer, osc->inbuf_length, x->osc_message, x->debug );
						finished = true; // We're done now if we had received any characters
            x->incomingPacketStarted = false;
					}
					else // skipping all starting END bytes
					{
						x->incomingPacketStarted = true;
            finished = false;
            osc->inBufferPointer = osc->inBuffer;
						osc->inbuf_length = 0;
					}
          //post("END! started %d, finished %d", x->incomingPacketStarted, finished);
					break;					
				case ESC:
          // get the next byte.  if it's not an ESC_END or ESC_ESC, it's a 
          // malformed packet.  http://tools.ietf.org/html/rfc1055 says just 
          // drop it in the packet in this case
          x->usbReadBufPtr++;
					x->usbReadBufLength--;
          if( x->incomingPacketStarted )
          {
            if( *(uchar*)x->usbReadBufPtr == ESC_END )
            {
              *osc->inBufferPointer++ = END;
						  osc->inbuf_length++;
              break;
            }
            else if( *(uchar*)x->usbReadBufPtr == ESC_ESC )
            {
              *osc->inBufferPointer++ = ESC;
						  osc->inbuf_length++;
              break;
            }
          }
					// no break here
				default:
					if( x->incomingPacketStarted )
					{
						*osc->inBufferPointer++ = *x->usbReadBufPtr;
						osc->inbuf_length++;
					}
			}
			x->usbReadBufPtr++;
			x->usbReadBufLength--;
      if( osc->inbuf_length > OSC_MAX_MESSAGE ) // if we ran out of space, reset.
      {
        osc->inBufferPointer = osc->inBuffer;
        osc->inbuf_length = 0;
        x->incomingPacketStarted = false;
        x->usbReadBufLength = 0;
      }
		}
	}
	return;
}


mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length )
{
	char *ptr;
	int size;
	char buf[ OSC_MAX_MESSAGE * 2 ]; // make it twice as long, as worst case scenario is ALL escape characters
	buf[0] = END;  // Flush out any spurious data that may have accumulated
	ptr = buf + 1; 
  size = (length > OSC_MAX_MESSAGE) ? OSC_MAX_MESSAGE : length;

  while( size-- )
  {
    switch(*(uchar*)packet)
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				*ptr++ = ESC;
				*ptr++ = ESC_END;
				break;
				
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				*ptr++ = ESC;
				*ptr++ = ESC_ESC;
				break;
				//otherwise, just send the character
			default:
				*ptr++ = *packet;
		}
		packet++;
	}
	
	// tell the receiver that we're done sending the packet
	*ptr++ = END;
	usb_write( u, buf, (ptr - buf) );
	Osc_resetOutBuffer( x->Osc );
	return 0;
}

// print out the device's file path, if it's connected, in response to a "filepath" message
void mcUsb_devicepath( t_mcUsb *x )
{
	if( x->mc_usbInt->deviceOpen )
	  post( "mc.usb is connected to a Make Controller at %s", x->mc_usbInt->deviceLocation );
	else
		post( "mc.usb is not currently connected to a Make Controller Kit." );
}

/*
  set mc.usb into a more verbose debug mode
*/
void mcUsb_debug( t_mcUsb *x, t_symbol *s, short ac, t_atom *av )
{
  if( ac == 1 )
  {
    int debug = atom_getlong(av);
    if( debug && !x->debug )
    {
      x->debug = debug;
      x->mc_usbInt->debug = debug;
      post("mc.usb: debug mode is on");
    }
    else if( !debug && x->debug )
    {
      x->debug = debug;
      x->mc_usbInt->debug = debug;
      post("mc.usb: debug mode is off");
    }
  }
}

void mcUsb_free(t_mcUsb *x)
{
  freeobject( (t_object*)x->mc_clock );
  freeobject( (t_object*)x->usb_clock );
  free( (t_osc*)x->Osc );
  free( (t_osc_message*)x->osc_message );
  mcusb_obj_count--;
  if( x->mc_usbInt->deviceOpen )
  {
    usb_close( x->mc_usbInt );
    post( "mc.usb closed the Make Controller Kit USB connection." );
  }
}

// the constructor for the object
void *mcUsb_new( t_symbol *s, long ac, t_atom *av )
{
	t_mcUsb* new_mcUsb;
  
  if( mcusb_obj_count )
  {
    post("note: only one mc.usb is allowed per patch - new object not created.");
    return NULL;
  }
  else
    mcusb_obj_count++;

	if( new_mcUsb = (t_mcUsb*)object_alloc(mcUsb_class) )
		new_mcUsb->out0 = outlet_new(new_mcUsb, 0L);  // add a left outlet
	
	new_mcUsb->mc_clock = clock_new( new_mcUsb, (method)mcUsb_tick );
	clock_delay( new_mcUsb->mc_clock, 1 );  //set the clock running every millisecond
  
  new_mcUsb->usb_clock = clock_new( new_mcUsb, (method)usb_tick );
	clock_delay( new_mcUsb->usb_clock, 250);  //set the clock running once a second
	
	new_mcUsb->Osc = ( t_osc* )malloc( sizeof( t_osc ) );
	Osc_resetOutBuffer( new_mcUsb->Osc );
	
	new_mcUsb->osc_message = ( t_osc_message* )malloc( sizeof( t_osc_message ) );
	Osc_reset_message( new_mcUsb->osc_message );
  new_mcUsb->incomingPacketStarted = false;

	new_mcUsb->usbReadBufPtr = new_mcUsb->usbReadBuffer;
	new_mcUsb->usbReadBufLength = 0;
  new_mcUsb->debug = false;
	
	new_mcUsb->mc_usbInt = usb_init( &new_mcUsb->mc_usbInt );
  usb_tick( new_mcUsb );
  	
	return( new_mcUsb );
}

