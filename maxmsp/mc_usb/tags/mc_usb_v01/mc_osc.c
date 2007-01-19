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

#include "mc_osc.h"
#include "ext.h" //for calling post() to the Max window.
#include "ext_obex.h"
#include "ext_common.h"

#ifndef WIN32
#include <CoreFoundation/CoreFoundation.h> // for CFByteOrder.h
#endif

#define OSC_MAX_MESSAGE 2048

char* writePaddedString( char* buffer, int* length, char* string )
{
  int tagLen = strlen( string ) + 1;
  int tagPadLen = tagLen;
  int pad = ( tagPadLen ) % 4;

  int i;

  if ( pad != 0 )
    tagPadLen += ( 4 - pad );
 
  *length -= tagPadLen;

  
  if ( *length >= 0 )
  {
    strcpy( buffer, string );
    buffer += tagLen;
    for ( i = tagLen; i < tagPadLen; i++ )
      *buffer++ = 0;
  }
  else
	return NULL;

  return buffer;
}

// Osc transmits bytes in big endian format.
// ...must change them if we're on a little endian machine.
unsigned int endianSwap( unsigned int a )
{
	#ifdef WIN32
		return ( ( a & 0x000000FF ) << 24 ) |
					( ( a & 0x0000FF00 ) << 8 )  | 
					( ( a & 0x00FF0000 ) >> 8 )  | 
					( ( a & 0xFF000000 ) >> 24 );
	#endif
	#ifndef WIN32 
		return CFSwapInt32BigToHost( (uint32_t)a );
	#endif
}

char* Osc_writeTimetag( t_osc* o, char* buffer, int* length, int a, int b )
{
  if ( *length < 8 )
    return NULL;

  *((int*)buffer) = endianSwap( a );
  buffer += 4;
  *((int*)buffer) = endianSwap( b );
  buffer += 4;
  *length -= 8;

  return buffer;
}

char* Osc_createBundle( t_osc* o, char* buffer, int* length, int a, int b )
{
  char *bp = buffer;

  // do the bundle bit
  bp = writePaddedString( bp, length, "#bundle" );
  if ( bp == NULL )
    return 0;

  // do the timetag
  bp = Osc_writeTimetag( o, bp, length, a, b );
  if ( bp == NULL )
    return 0;

  return bp;
}

int Osc_extract_data( t_osc* o, char* buffer, t_osc_message* osc_message )
{
  int count = 0;
  char* data;
  char* tp;
  t_atom* ap;
  bool cont;
  
  // figure out where the data starts
  int tagLen = strlen( buffer ) + 1;
  int pad = tagLen % 4;
  if ( pad != 0 )
    tagLen += ( 4 - pad );
  data = buffer + tagLen;

  // Going to be walking through the type tag, and the data.
   // need to skip the comma ','
  ap = osc_message->argv; // to walk along the data arguments
  cont = true;
  for( tp = buffer + 1; *tp && cont; tp++ )
  {
    cont = false;
    switch ( *tp )
    {
      case 'i':
	  {
        long i = *(long*)data;
        i = endianSwap( i );
		//post( "Integer: %d ", i );
		data += 4;
		count++;
		if ( osc_message->argc < OSC_MAX_ARG_COUNT )
		{
		  osc_message->argc++;
		  atom_setlong( ap, i );
		  ap++;
		  cont = true;
		}
        break;
	  }
      case 'f':
	  {
		int i = *(int*)data;
		float f;
        i = endianSwap( i );
        f = *(float*)&i;
		//post( "Float: %f ", f );
		data += 4;
		count++;
		if ( osc_message->argc < OSC_MAX_ARG_COUNT )
		{ 
  		  osc_message->argc++;
		  atom_setfloat( ap, f );
		  ap++;
		  cont = true;
		}
        break;
	  }
      case 's':
	  {
		//post( "String: %s ", data );
	    int pad;
		int len = strlen( data ) + 1;
		if ( osc_message->argc < OSC_MAX_ARG_COUNT )
		{
          osc_message->argc++;
		  atom_setsym( ap, gensym( data ) );
		  ap++;
		  pad = len % 4;
		  if ( pad != 0 )
		    len += ( 4 - pad );
		  data += len;
		  count++;
		  cont = true;
		}
		
        break;
	  }
	  default:
        break;
    }
  }
  return count;
}

// When we receive a packet, check to see whether it is a message or a bundle.
void Osc_receive_packet( void* out, t_osc* o, char* packet, int length, t_osc_message* osc_message )
{
	//printf( "Raw: %s, Length: %d\n", packet, length );
	switch( *packet )
	{
		case '/':		// the '/' in front tells us this is an Osc message.
			Osc_receive_message( o, packet, length, osc_message );
			outlet_anything( out, osc_message->address, osc_message->argc, osc_message->argv );
			Osc_reset_message( osc_message );
			break;
		case '#':		// the '#' tells us this is an Osc bundle, and we check for "#bundle" just to be sure.
			if ( strcmp( packet, "#bundle" ) == 0 )
      {
        // skip bundle text and timetag
        packet += 16;
        length -= 16;
        while ( length > 0 )
        {
          // read the length (pretend packet is a pointer to integer)
          int messageLength = endianSwap( *((int*)packet) );
					//post( "Unpacking bundle of length %d.", messageLength );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
            Osc_receive_packet( out, o, packet, messageLength, osc_message );
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
		default:
			// something we don't recognize...
			//message( 1, "%s> Error - Osc packets must start with either a '/' (message) or '[' (bundle).\n", preamble );
			break;
	}
}

/*
	Once we receive a message, we need to make sure it's in the right format,
	and then send it off to be interpreted (via extractData() ).
*/
void Osc_receive_message( t_osc* o, char* packet, int length, t_osc_message* osc_message )
{
  char* type;
  // We can print the address by just trying to print message, since it's null-terminated after the address.
  //message( 1, "%s> %s ", preamble, in );
  if ( osc_message != 0 )
    osc_message->address = gensym( packet );
	
  // Then try to find the type tag
  type = Osc_find_data_tag( o, packet, length );
  if ( type == NULL )		//If there was no type tag, say so and stop processing this message.
	post( "Error - No type tag." );
  else		//Otherwise, step through the type tag and print the data out accordingly.
  {
	//We get a count back from extractData() of how many items were included - if this
	//doesn't match the length of the type tag, something funky is happening.
	int count = Osc_extract_data( o, type, osc_message );
	if ( count != (int)( strlen(type) - 1 ) )
	  post( "Error extracting data from packet - type tag doesn't correspond to data included." );
  }
}

char* Osc_find_data_tag( t_osc* o, char* message, int length )
{
  while ( *message != ',' && length-- > 0 )
    message++;
  if ( length <= 0 )
    return NULL;
  else
    return message;
}

mcError Osc_create_message( t_osc* o, char* address, int ac, t_atom* av )
{  
  // try to send this message - if there's a problem somewhere, 
  // send the existing buffer - freeing up space, then try (once) again.
  int count = 0;
  char* bp;
  char* buffer;
  int length;
  int* lp;
  char* mp;
	
  do
  {  
    count++;

	buffer = o->outBufferPointer;
    length = o->outBufferRemaining;	
	bp = buffer;
  
    // First message in the buffer?
		/*
    if ( bp == o->outBuffer )
    {
      bp = Osc_create_bundle( o, bp, &length, 0, 0 );
      if ( bp == NULL )
        return MC_ERROR_CREATING_BUNDLE;
    }
  */
    // remember the place we're going to store the message length
    lp = (int *)bp;
    //bp += 4;
    //length -= 4;
    
    // remember the start of the message
    mp = bp;    
    if( length > 0 )
	  bp = Osc_create_message_internal( o, bp, &length, address, ac, av ); 
	else
	  bp = 0;
      
    if ( bp != 0 )
    {
      // Set the size
      //*lp = endianSwap( bp - mp );
	  o->outBufferPointer = bp;
      o->outBufferRemaining = length;
      o->outMessageCount++;
    }
    else
    {
			//return o->//Osc_send_packet( o, bp, (OSC_MAX_MESSAGE - outBufferRemaining) );
    }
  } while ( bp == 0 && count == 1 );

	return ( bp != 0 ) ? MC_OK : MC_ERROR_SENDING_TEXT_MESSAGE;
}

char* Osc_create_message_internal( t_osc* o, char* bp, int* length, char* address, int ac, t_atom* av )
{
  char typetag[128];
  char dataBuf[256]; //place to build up all the arguments while we're still figuring out the typetag
  char* typeP;
  char* tp;
  char* dataP;
  char* dp;
  int data_length;
  int i;
  t_symbol* s;
  int string_length;
  int val;

  // do the address
  bp = writePaddedString( bp, length, address );
  if ( bp == NULL )
    return 0;

  // Going to be walking the tag string, the format string and the data
  // skip the ',' comma
	
  typeP = typetag;
  tp = typetag;
  *tp = ','; // first char has to be a ,
  tp++;
	
  dataP = dataBuf;
  dp = dataBuf;
  data_length = 0;
	
  // use the argc and argv that Max gives us to create the typetag and the data arguments
  // hold them in temporary places (since we don't know how long they are and therefore don't know
  // how we're going to need to pad the typetag).
  for( i = 0; i < ac; i++ )
  {
	switch( av->a_type )
	{
	  case A_SYM:
		// stick things in the typetag
		*tp = 's';
		tp++;
		// stick things in the data string
		s = atom_getsym( av );
		string_length = *length; //grab this so we can check it after writePaddedString
		dp = writePaddedString( dp, length, s->s_name );
		string_length -= *length; //find out how long the string was
		data_length += string_length;
	    break;
	  case A_LONG:
	    *length -= 4;
		if( *length >= 0 )
		{
		  // stick things in the typetag
		  *tp = 'i';
		  tp++;
		  // stick things in the data string
		  val = (int)atom_getlong( av );
		  val = endianSwap( val );
		  //post( "Putting this int into data buffer: %d", val );
		  *((int*)dp) = val;
		  dp += 4;
		  data_length += 4;
		}
		break;
	  case A_FLOAT:
		*length -= 4;
		if( *length >= 0 )
		{
		  // stick things in the typetag
		  *tp = 'f';
		  tp++;
		  // stick things in the data string
		  *((float*)dp) = atom_getfloat( av );
		  dp += 4;
		  data_length += 4;
		}
		break;
	  default:
		//post( "first message was something else." );
		break;
	}
	av++;
  }
  *tp = '\0'; //terminate the typetag
  //post( "Typetag: %s", typeP );
  *dp = '\0'; //terminate the data string
	
  // do the type
  bp = writePaddedString( bp, length, typeP );
  if ( bp == NULL )
    return 0;
		
  // do the data.
  //post( "Data: %s, data length: %d", dataP, data_length );
  memcpy( bp, dataP, data_length );

  return bp;
}

void Osc_resetOutBuffer( t_osc* o )
{
  o->outBufferPointer = o->outBuffer;
  o->outBufferRemaining = OSC_MAX_MESSAGE;
  o->outMessageCount = 0;
}

void Osc_reset_message( t_osc_message* msg )
{
  msg->address = gensym( "NULL" );
	msg->argc = 0;
}


