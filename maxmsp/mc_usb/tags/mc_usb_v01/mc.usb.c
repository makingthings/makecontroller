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

/* mc.usb.c
*
* MakingThings 2006.
*/	

#include "ext.h"
#include "ext_common.h"
#include "ext_obex.h"

#include "mcError.h"
#include "usb_serial.h"
#include "mc_osc.h"

#define MAXSIZE 512

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

// Max object structure def.
typedef struct _mcUsb
{
  t_object mcUsb_ob;
  void* obex;
  t_symbol *symval;
  //Max things
  void* mc_clock;
  long sampleperiod; //the sampleperiod attribute
  void *out0;
  //OSC things
  t_osc_packet* osc_packet;  //the current packet being created
  char* packetP;  //the pointer to the beginning of the packet
  t_osc_message* osc_message; // the message to be sent out into Max
  t_osc* Osc;  //the osc object
  int packetStarted;
  // USB things
  t_usbInterface* mc_usbInt;  // the USB interface
  char* usbIncomingBuf;  // the 
  char singleChar;
} t_mcUsb;

void *mcUsb_class;  // global variable pointing to the class

void *mcUsb_new( t_symbol *s, long ac, t_atom *av );
void mcUsb_free(t_mcUsb *x);
void mcUsb_bang(t_mcUsb *x);
void mcUsb_int(t_mcUsb *x, long c);
void mcUsb_anything(t_mcUsb *x, t_symbol *s, short ac, t_atom *av);
void mcUsb_assist(t_mcUsb *x, void *b, long m, long a, char *s);
void mcUsb_tick( t_mcUsb *x );
mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length );
void mc_build_packet( t_mcUsb *x, char c );
void mcUsb_sampleperiod( t_mcUsb *x, long i );

// pattr support function prototypes
//t_max_err mcUsb_getvalueof(t_mcUsb *x, long *ac, t_atom **av);
//t_max_err mcUsb_setvalueof(t_mcUsb *x, long ac, t_atom *av);

// define the object so Max knows all about it, and define which messages it will implement and respond to
int main( )
{
  t_class	*c;
	void	*attr;
	long	attrflags = 0;
	
	c = class_new("mcUsb", (method)mcUsb_new, (method)mcUsb_free, (short)sizeof(t_mcUsb), 0L, A_GIMME, 0);
	
	//common_symbols_init( );  // initialize the common symbols, since we want to use them
	
	class_obexoffset_set(c, calcoffset(t_mcUsb, obex));  // register the byte offset of obex with the class
	
	//add some attributes
	attr = attr_offset_new("sampleperiod", gensym( "long" ), attrflags, 
		(method)0L, (method)0L, calcoffset(t_mcUsb, sampleperiod));
	class_addattr(c, attr);

	class_addmethod(c, (method)mcUsb_bang, "bang", 0);
	class_addmethod(c, (method)mcUsb_int, "int", A_LONG, 0);
	class_addmethod(c, (method)mcUsb_anything, "anything", A_GIMME, 0);
	class_addmethod(c, (method)mcUsb_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)mcUsb_sampleperiod, "sampleperiod", A_LONG,0);
	
	// these methods support the pattr set of objects
	//class_addmethod(c, (method)mcUsb_getvalueof, "getvalueof", A_CANT, 0);  
	//class_addmethod(c, (method)mcUsb_setvalueof, "setvalueof", A_CANT, 0);

	// add methods for dumpout and quickref	
	class_addmethod(c, (method)object_obex_dumpout, "dumpout", A_CANT, 0); 
	class_addmethod(c, (method)object_obex_quickref, "quickref", A_CANT, 0);

	// we want this class to instantiate inside of the Max UI; ergo CLASS_BOX
	class_register(CLASS_BOX, c);
	mcUsb_class = c;
	return 0;
}

void mcUsb_bang(t_mcUsb *x)
{
	;
}

void mcUsb_int(t_mcUsb *x, long c)
{
	post( "mcUsb got an int: %d", c );
	object_notify( (void*)x, gensym( "int" ), (void*)c );
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
	int readResult;
	//post( "tick." );

	while( 1 )
	{
	  readResult = usb_read( x->mc_usbInt, &x->singleChar, 1 );  //we're only ever going to read 1 character at a time
	  if( readResult != MC_GOT_CHAR ) //we got a character
	    break;
	  mc_build_packet( x, x->singleChar );
	}
	if( x->mc_usbInt->deviceOpen )
	  clock_delay( x->mc_clock, x->sampleperiod ); //set the delay interval for the next tick
	else
	  clock_delay( x->mc_clock, 1000 );
	  
}

void mc_build_packet( t_mcUsb *x, char c )
{
  int readResult;
	char d;
	switch( (unsigned char)c )
	{
		case END:
			if( x->packetStarted && x->osc_packet->length ) // it was the END byte
			{ 
				*x->packetP = '\0';
				Osc_receive_packet( x->out0, x->Osc, x->osc_packet->packetBuf, x->osc_packet->length, x->osc_message );
				x->packetStarted = 0;
			} 
			else // it was the START byte
			{
				x->packetP = x->osc_packet->packetBuf;
				x->osc_packet->length = 0;
				x->packetStarted = 1;
			}
			break;
			// if it's the same code as an ESC character, get another character,
			// then figure out what to store in the packet based on that.
		case ESC:
			readResult = usb_read( x->mc_usbInt, &d, 1 );
			if( readResult == MC_GOT_CHAR )
			{
				switch( (unsigned char)c )
				{
					case ESC_END:
						c = END;
						break;
					case ESC_ESC:
						c = ESC;
						break;
				}
			}
			else
				break;
			// otherwise, just stick it in the packet		
		default:
			if( x->packetP == NULL )
				break;
			*x->packetP = c;
		  //post( "Just read this: %c", c );
			x->packetP++;
			x->osc_packet->length++;
			//*x->packetP = 0;
			//post( "packet length: %d", x->osc_packet->length );
			//post( "Packet: %s", x->osc_packet->packetBuf );
	}
}

mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length )
{
	  /*
	  // see if we can dispense with the bundle business
	  if ( outMessageCount == 1 )
	  {
	    // skip 8 bytes of "#bundle" and 8 bytes of timetag and 4 bytes of size
	    buffer += 20;
	    // shorter too
	    length -= 20;
	  }
	  */
		//packetInterface->sendPacket( buffer, size );
		
	int size = length;
  usb_writeChar( u, END ); // Flush out any spurious data that may have accumulated

  while( size-- )
  {
    switch( (unsigned char)*packet )
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				usb_writeChar( u, ESC );
				usb_writeChar( u, ESC_END );
				break;
				
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				usb_writeChar( u, ESC );
				usb_writeChar( u, ESC_ESC );
				break;
				//otherwise, just send the character
			default:
				usb_writeChar( u, *packet );
			  //post( "Just wrote %c", *packet );
				break;
		}
		packet++;
	}
	// tell the receiver that we're done sending the packet
	usb_writeChar( u, END );
	Osc_resetOutBuffer( x->Osc );
	return MC_OK;
}

// set how often the USB port should be read.
void mcUsb_sampleperiod( t_mcUsb *x, long i )
{
	if( i < 1 )
	  i = 1;
	object_attr_setlong( x, gensym("sampleperiod"), i );
}

void mcUsb_free(t_mcUsb *x)
{
  freeobject( (t_object*)x->mc_clock );
  free( (t_osc_packet*)x->osc_packet );
  free( (t_osc*)x->Osc );
  free( (t_osc_message*)x->osc_message );
  usb_close( x->mc_usbInt );
}

// the constructor for the object
void *mcUsb_new( t_symbol *s, long ac, t_atom *av )
{
	t_mcUsb* new_mcUsb;
	int i;
	cchar* usbConn = NULL;

	// we use object_alloc here, rather than newobject
	if( new_mcUsb = (t_mcUsb*)object_alloc(mcUsb_class) )
	{
		//object_obex_store( new_mcUsb, _sym_dumpout, outlet_new(new_mcUsb, NULL) ); // add a dumpout outlet
		new_mcUsb->out0 = outlet_new(new_mcUsb, 0L);  // add a left outlet
		
		//new_mcUsb->symval->s_thing = (t_object*)new_mcUsb;
		
		//for( i = 0; i < ac; i++ )
		 //post( "Argument %d: %s\n", i, atom_getsym( av + i ));

		//object_register( gensym( "_makeController" ), gensym( "_mcUsb" ), new_mcUsb );
		
		//attr_args_process(new_mcUsb, ac, av);		
	}
	
	new_mcUsb->mc_clock = clock_new( new_mcUsb, (method)mcUsb_tick );
	new_mcUsb->sampleperiod = 1;  //interval at which to call the clock
	clock_delay( new_mcUsb->mc_clock, new_mcUsb->sampleperiod );  //set the clock running
	
	new_mcUsb->Osc = ( t_osc* )malloc( sizeof( t_osc ) );
	Osc_resetOutBuffer( new_mcUsb->Osc );
	
	new_mcUsb->osc_packet = ( t_osc_packet* )malloc( sizeof( t_osc_packet ));
	new_mcUsb->packetStarted = 0;
	new_mcUsb->packetP = NULL;
	
	new_mcUsb->osc_message = ( t_osc_message* )malloc( sizeof( t_osc_message ) );
	Osc_reset_message( new_mcUsb->osc_message );
	
	new_mcUsb->mc_usbInt = usb_init( usbConn, &new_mcUsb->mc_usbInt );
	usb_open( new_mcUsb->mc_usbInt );
	
	return( new_mcUsb );
}

