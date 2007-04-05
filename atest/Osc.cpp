/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "Osc.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include <stdlib.h>

/*
 * Constructor for the Osc subsystem
 */
Osc::Osc( PacketInterface* packetInterface, MessageInterface *messageInterface )
{
  this->packetInterface = packetInterface;  
  this->messageInterface = messageInterface;	

	resetOutBuffer( );
	
	// some other stuff
	
	// testing automake

}

bool Osc::isMessageWaiting()
{
  return packetInterface->isPacketWaiting();
}

Osc::Status Osc::receive( OscMessage* oscMessage )
{
  char buffer[ 1024 ];
  
  if ( packetInterface->isPacketWaiting() )
  {
	  if ( oscMessage!= 0 )
	  {
	    oscMessage->address = 0;
	    oscMessage->s = 0;
	    oscMessage->i = 0;
	    oscMessage->f = 0.0;
	  }
	  int length = packetInterface->receivePacket( buffer, 1024 );
	  if ( length == 0 )
	  {
  		messageInterface->message( 2, "  OSC Length 0\n" );
  	  return ERROR_PACKET_LENGTH_0;
	  }
	    
	  receivePacket( buffer, length, oscMessage );
	  
	  return OK;
  }
  else
  {
    return ERROR_NO_PACKET;
  }
}

/*
	An OSC Type Tag String is an OSC-string beginning with the character ',' (comma) followed by a sequence 
	of characters corresponding exactly to the sequence of OSC Arguments in the given message. 
	Each character after the comma is called an OSC Type Tag and represents the type of the corresponding OSC Argument. 
	(The requirement for OSC Type Tag Strings to start with a comma makes it easier for the recipient of an OSC Message 
	to determine whether that OSC Message is lacking an OSC Type Tag String.)
	
	OSC Type Tag		Type of corresponding argument
			i						int32
			f						float32
			s						Osc-string
			b						Osc-blob
*/
char* Osc::findDataTag( char* message, int length )
{
  while ( *message != ',' && length-- > 0 )
    message++;
  if ( length <= 0 )
    return NULL;
  else
    return message;
}

// When we receive a packet, check to see whether it is a message or a bundle.
void Osc::receivePacket( char* packet, int length, OscMessage* oscMessage )
{
	//printf( "Raw: %s, Length: %d\n", packet, length );
	switch( *packet )
	{
		case '/':		// the '/' in front tells us this is an Osc message.
			receiveMessage( packet, length, oscMessage );
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
          int messageLength = this->endianSwap( *((int*)packet) );
					printf( "Unpacking bundle of length %d.\n", messageLength );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
            receivePacket( packet, messageLength, oscMessage );
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
		default:
			// something we don't recognize...
			messageInterface->message( 3, "osc> Error - Osc packets must start with either a '/' (message) or '[' (bundle).\n" ); 
	}
}

/*
	Once we receive a message, we need to make sure it's in the right format,
	and then send it off to be interpreted (via extractData() ).
*/
void Osc::receiveMessage( char* message, int length, OscMessage* oscMessage )
{
	// We can print the address by just trying to print message, since it's null-terminated after the address.
	messageInterface->message( 3, "  Message In: %s ", message );
	if ( oscMessage != 0 )
	  oscMessage->address = strdup( message );
	
	// Then try to find the type tag
	char* type = findDataTag( message, length );
  if ( type == NULL )		//If there was no type tag, say so and stop processing this message.
	{
		messageInterface->message( 3, "\n    Error - No type tag.\n" );
	} 
	else		//Otherwise, step through the type tag and print the data out accordingly.
	{
		/*
		mainDialog->writeToConsole( " Type Tag: " );
		mainDialog->writeToConsole( type );
		mainDialog->writeToConsole( " " );
		*/
		
		//We get a count back from extractData() of how many items were included - if this
		//doesn't match the length of the type tag, something funky is happening.
		int count = extractData( type, oscMessage );
		if ( count != (int)( strlen(type) - 1 ) )
		{
			messageInterface->message( 3, "\n    Error extracting data from packet - type tag doesn't correspond to data included.\n" );
		}
	}
}

/*
	Once we're finally in our message, we need to read the actual data.
	This means we need to step through the type tag, and then step the corresponding number of bytes
	through the data, depending on the type specified in the tag.
*/
int Osc::extractData( char* buffer, OscMessage* oscMessage )
{
  int count = 0;

  // figure out where the data starts
  int tagLen = strlen( buffer ) + 1;
  int pad = tagLen % 4;
  if ( pad != 0 )
    tagLen += ( 4 - pad );
  char* data = buffer + tagLen;

  // Going to be walking through the type tag, and the data.
  char* tp; // need to skip the comma ','
  bool cont = true;
  for ( tp = buffer + 1; *tp && cont; tp++ )
  {
    cont = false;
    switch ( *tp )
    {
      case 'i':
      {
        int i = *(int*)data;
        i = endianSwap( i );
				messageInterface->message( 3, "%d ", i );
				data += 4;
				count++;
				if ( oscMessage) 
				  oscMessage->i = i;
				cont = true;
        break;
      }
      case 'f':
      {
				int i = *(int*)data;
        i = endianSwap( i );
        float f = *(float*)&i;
				messageInterface->message( 3, "%f ", f );
				data += 4;
				count++;
				if ( oscMessage) 
  				oscMessage->f = f;
				cont = true;
        break;
      }
      case 's':
      {
				messageInterface->message( 3, "%s ", data );
				int len = strlen( data ) + 1;
				int pad = len % 4;
				if ( pad != 0 )
					len += ( 4 - pad );
				data += len;
				count++;
				cont = true;
        break;
      }
    }
  }
	messageInterface->message( 3, "\n" );
  return count;
}

char* Osc::createBundle( char* buffer, int* length, int a, int b )
{
  char *bp = buffer;

  // do the bundle bit
  bp = this->writePaddedString( bp, length, "#bundle" );
  if ( bp == NULL )
    return 0;

  // do the timetag
  bp = this->writeTimetag( bp, length, a, b );
  if ( bp == NULL )
    return 0;

  return bp;
}

/**
  createMessage
  Must put the "," as the first format letter
  */
Osc::Status Osc::createMessage( char* address, char* format, ... )
{  
  // try to send this message - if there's a problem somewhere, 
  // send the existing buffer - freeing up space, then try (once) again.
  int count = 0;
  char *bp;
  do
  {  
    count++;

    char* buffer = outBufferPointer;
    int length = outBufferRemaining;
  
    bp = buffer;
  
    // First message in the buffer?
    if ( bp == outBuffer )
    {
      bp = createBundle( bp, &length, 0, 0 );
      if ( bp == NULL )
        return ERROR_CREATING_BUNDLE;
    }
  
 
    // remember the place we're going to store the message length
    int* lp = (int *)bp;
    bp += 4;
    length -= 4;

    // remember the start of the message
    char* mp = bp;    

    if ( length > 0 )
    {      
      // Set up to iterate through the arguments
      va_list args;
      va_start( args, format );
    
      bp = createMessageInternal( bp, &length, address, format, args ); 

      va_end( args );
    }
    else
      bp = 0;
      
    if ( bp != 0 )
    {
      // Set the size
      *lp = endianSwap( bp - mp ); 
  
      outBufferPointer = bp;
      outBufferRemaining = length;
      outMessageCount++;
    }
    else
    {
      sendPacket( );
    }
  } while ( bp == 0 && count == 1 );

  return ( bp != 0 ) ? OK : ERROR_SENDING_TEXT_MESSAGE;
}


/**
  createMessage
  Must put the "," as the first format letter
  */
Osc::Status Osc::createMessage( char* textMessageOriginal )
{  
	char* textMessage = strdup( textMessageOriginal );
	
  // try to send this message - if there's a problem somewhere, 
  // send the existing buffer - freeing up space, then try (once) again.
  int count = 0;
  char *bp;
  do
  {  
    count++;

    char* buffer = outBufferPointer;
    int remaining = outBufferRemaining;
  
    bp = buffer;
  
    // First message in the buffer?
    if ( bp == outBuffer )
    {
      bp = createBundle( bp, &remaining, 0, 0 );
      if ( bp == NULL )
        return ERROR_CREATING_BUNDLE;
    }
  
    // remember the place we're going to store the message length
    int* lp = (int *)bp;
    bp += 4;
    remaining -= 4;

    // remember the start of the message
    char* mp = bp;    

    if ( remaining > 0 )
    {          
      bp = createMessageInternal( bp, &remaining, textMessage ); 
    }
    else
      bp = 0;
      
    if ( bp != 0 )
    {
      // Set the size
      *lp = endianSwap( bp - mp ); 
  
      outBufferPointer = bp;
      outBufferRemaining = remaining;
      outMessageCount++;
    }
    else
    {
      sendPacket( );
    }
  } while ( bp == 0 && count == 1 );

  free( textMessage );

  return ( bp != 0 ) ? OK : ERROR_SENDING_COMPLEX_MESSAGE;
}


char* Osc::createMessageInternal( char* bp, int* length, char* address, char* format, va_list args )
{
  // do the address
  bp = writePaddedString( bp, length, address );
  if ( bp == NULL )
    return 0;

  // do the type
  bp = writePaddedString( bp, length, format );
  if ( bp == NULL )
    return 0;

  // Going to be walking the tag string, the format string and the data
  // skip the ',' comma
  char* fp;
  bool cont = true;
  for ( fp = format + 1; *fp && cont; fp++ )
  {
    switch ( *fp )
    {
      case 'i':
          *length -= 4;
          if ( *length >= 0 )
          {
            int v = va_arg( args, int );
            v = this->endianSwap( v );
            *((int*)bp) = v;
            bp += 4;
          }
          else 
            cont = false;
        break;
      case 'f':
        *length -= 4;
        if ( *length >= 0 )
        {
          int v;
          *((float*)&v) = (float)( va_arg( args, double ) ); 
          v = endianSwap( v );
          *((int*)bp) = v;
          bp += 4;
        }
        else 
          cont = false;
        break;
      case 's':
      {
        char* s = va_arg( args, char* );
        bp = this->writePaddedString( bp, length, s );
        if ( bp == NULL )
          cont = false;
        break;
      }
      default:
        cont = false;
    }
  }

  return ( cont ) ? bp : NULL;
}

Osc::Status Osc::sendPacket( )
{
	if ( outMessageCount > 0 )
	{
	  // set the buffer and length up
	  char* buffer = outBuffer;
	  int length = OSC_MAX_MESSAGE - outBufferRemaining;
	
	  // see if we can dispense with the bundle business
	  if ( outMessageCount == 1 )
	  {
	    // skip 8 bytes of "#bundle" and 8 bytes of timetag and 4 bytes of size
	    buffer += 20;
	    // shorter too
	    length -= 20;
	  }
	
		packetInterface->sendPacket( buffer, length );
	}
	
  resetOutBuffer( );
	
	return OK;
}

char* Osc::writePaddedString( char* buffer, int* length, char* string )
{
  int tagLen = strlen( string ) + 1;
  int tagPadLen = tagLen;
  int pad = ( tagPadLen ) % 4;
  if ( pad != 0 )
    tagPadLen += ( 4 - pad );
 
  *length -= tagPadLen;

  if ( *length >= 0 )
  {
    strcpy( buffer, string );
    int i;
    buffer += tagLen;
    for ( i = tagLen; i < tagPadLen; i++ ) 
      *buffer++ = 0;
  }
  else
    return NULL;

  return buffer;
}

char* Osc::writeTimetag( char* buffer, int* length, int a, int b )
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

// pass the text entry from the main command line, 
// guess the type of the arguments, 
// and create an appropriate type tag
char* Osc::createMessageInternal( char* bp, int* remaining, char* inputString )
{
	char* ip;	// pointer into the input string
	char typetag[ 32 ];				//intermediate buffer for typetag
	char argumentData[ 256 ];	//intermediate buffer for args/data
	char* ap = argumentData;	// pointer into the argument buffer
	
	bool nullified;
	
	//messageInterface->message( 3, "Initial Remaining %d\n", *remaining );

	strcpy( typetag, "," );	//always needs to start with a comma
	
	// then we need to get the address out of the way so we can get to the arguments
	for( ip = inputString; *ip != ' ' && *ip != 0; ip++ )
		;
	*ip = 0;	//put a null terminator in the string so we can grab the front end and stuff it in the message
	ip++;
	
	bp = writePaddedString( bp, remaining, inputString );

	//messageInterface->message( 3, "Length After Address %d\n", *remaining );

	while( *ip == ' ' )
		ip++;	//get rid of any spaces between the address and the arguments in the user's command
		
	// now we're at the arguments
	char* startpoint;
	
	// try to pack all the parameters, don't worry about length in here, it's 
	// handled below
	do
	{
		int gotDecimals = 0;
		bool gotCharacter = false;
		nullified = false;
		
		// look through each character of the argument, and set the appropriate flags to tell us what kind of argument it is
		for( startpoint = ip; *ip != ' ' && *ip != 0; ip++ )
		{
			if( *ip == '.' )
				gotDecimals++;
			else if( !isdigit( *ip ) )
				gotCharacter = true;
		}
		// if we're not at the end of the string, we need to grab just this argument
		if( *ip == ' ' )
		{
			*ip = 0;	//null-terminate the first argument so we can grab it
			nullified = true;
			ip++;	//make sure to push to the next character so next round through we're not evaluating the one we just set to null
		}
		//evaluate the type of data, then stuff it into the argument buffer, and increment the buffer pointer
		if( gotCharacter || gotDecimals > 1 )
		{
			strcat( typetag, "s" );
			int dummy = 1000;
			ap = writePaddedString( ap, &dummy, startpoint );
		}
		else if( gotDecimals == 1 )
		{
			strcat( typetag, "f" );
			float floatArgument = 0.0;
			sscanf( startpoint, "%f", &floatArgument );
			//messageInterface->message( 3, "Trying to get a float from %s.  Got %f\n", startpoint, floatArgument );
      unsigned int v;
      *((float*)&v) = floatArgument; 
      v = endianSwap( v );
      *((int*)ap) = v;
			ap += 4;
		}
		else if( strlen( startpoint ) > 0 )
		{
			strcat( typetag, "i" );
			int intArgument;
			sscanf( startpoint, "%d", &intArgument );
			*((int*)ap) = endianSwap( intArgument );
			ap += 4;		
		}
		else
		{ 
		} //do nothing
		
	} while( *ip != 0 || nullified );
		
	// now write the type tag and the arguments into the outgoing message buffer to create the full Osc message
	bp = writePaddedString( bp, remaining, typetag );
	int dataCopyLength = ap - argumentData;
	//messageInterface->message( 3, "Data length %d\n", dataCopyLength );
	// arguments are all already byte-aligned - shove them in as they are
	memcpy( bp, argumentData, dataCopyLength );
	*remaining -= dataCopyLength;
	//messageInterface->message( 3, "Length After Data %d\n", *remaining );
	bp+=dataCopyLength;
	
	return bp;
}

void Osc::resetOutBuffer( )
{
  outBufferPointer = outBuffer;
  outBufferRemaining = OSC_MAX_MESSAGE;
  outMessageCount = 0;
}

// Osc transmits bytes in big endian format.
// ...must change them if we're on a little endian machine.
unsigned int Osc::endianSwap( unsigned int a )
{
	#ifdef __LITTLE_ENDIAN__
		return ( ( a & 0x000000FF ) << 24 ) |
					( ( a & 0x0000FF00 ) << 8 )  | 
					( ( a & 0x00FF0000 ) >> 8 )  | 
					( ( a & 0xFF000000 ) >> 24 );
	#elif __BIG_ENDIAN__
		return a;
	#endif
}

