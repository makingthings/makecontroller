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

#include "Osc.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include <stdlib.h>

QString OscMessage::toString( )
{
	QString ret = QString( address );
	int j;
	for( j = 0; j < data.size( ); j++ )
	{
		ret.append( " " );
		switch( data.at( j )->omdType )
		{
			case OscMessageData::OmdBlob:
				ret.append( (char*)data.at( j )->b );
				break;
			case OscMessageData::OmdString:
				ret.append( data.at( j )->s );
				break;
			case OscMessageData::OmdInt:
				ret.append( QString::number(data.at( j )->i) );
				break;
			case OscMessageData::OmdFloat:
				ret.append( QString::number(data.at( j )->f) );
				break;
		}
	}
	return ret;
}

Osc::Osc( )
{
	resetOutBuffer( );
	preamble = NULL;
	packetInterface = NULL;
}

Osc::~Osc( )
{
	if( packetInterface != NULL )
	{
		if( packetInterface->isOpen( ) )
			packetInterface->close( );
		delete packetInterface;
	}
}

const char* Osc::getPreamble( )
{ 
	return preamble;
} 

Osc::Status Osc::receive( QList<OscMessage*>* oscMessageList )
{
  char buffer[ 1024 ];
  
	if ( packetInterface->isPacketWaiting() )
  {
		int length;
		length = packetInterface->receivePacket( buffer, 1024 );
		if ( length == 0 )
			return ERROR_PACKET_LENGTH_0;
		
		receivePacket( buffer, length, oscMessageList );
		return OK;
	}
	else
    return ERROR_NO_PACKET;
}

void Osc::setInterfaces( PacketInterface* packetInterface, MessageInterface* messageInterface, QApplication* application )
{
	this->packetInterface = packetInterface;  
  this->messageInterface = messageInterface;
	this->application = application;
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
void Osc::receivePacket( char* packet, int length, QList<OscMessage*>* oscMessageList )
{
	//printf( "Raw: %s, Length: %d\n", packet, length );
	switch( *packet )
	{
		case '/':		// the '/' in front tells us this is an Osc message.
			receiveMessage( packet, length, oscMessageList );
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
          int messageLength = qFromBigEndian( *((int*)packet) );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
            receivePacket( packet, messageLength, oscMessageList );
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
		default:
			// something we don't recognize...
			QString msg = QString( "Error - Osc packets must start with either a '/' (message) or '[' (bundle).");
			messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
	}
}

/*
	Once we receive a message, we need to make sure it's in the right format,
	and then send it off to be interpreted (via extractData() ).
*/
void Osc::receiveMessage( char* in, int length, QList<OscMessage*>* oscMessageList )
{
	OscMessage* oscMessage = new OscMessage( );
	oscMessage->address = strdup( in );
	
	// Then try to find the type tag
	char* type = findDataTag( in, length );
  if ( type == NULL )		//If there was no type tag, say so and stop processing this message.
  {
		QString msg = QString( "Error - No type tag.");
		messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
		delete oscMessage;
  }
	else		//Otherwise, step through the type tag and print the data out accordingly.
	{
		//We get a count back from extractData() of how many items were included - if this
		//doesn't match the length of the type tag, something funky is happening.
		int count = extractData( type, oscMessage );
		if ( count != (int)( strlen(type) - 1 ) )
		{
			QString msg = QString( "Error extracting data from packet - type tag doesn't correspond to data included.");
			messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
			delete oscMessage;
		}
		else
			oscMessageList->append( oscMessage );
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
        i = qFromBigEndian( i );
		//message( 1, "%d ", i );
		data += 4;
		count++;
		if ( oscMessage )
		{
		  OscMessageData* omdata = new OscMessageData( );
		  omdata->i = i;
		  omdata->omdType = OscMessageData::OmdInt;
		  oscMessage->data.append( omdata );
		}
		cont = true;
        break;
      }
      case 'f':
      {
		int i = *(int*)data;
        i = qFromBigEndian( i );
        float f = *(float*)&i;
        if ( oscMessage)
		{
		  OscMessageData* omdata = new OscMessageData( );
		  omdata->f = f;
		  omdata->omdType = OscMessageData::OmdFloat;
		  oscMessage->data.append( omdata );
		}
		//message( 1, "%f ", f );
		data += 4;
		count++;
		cont = true;
        break;
      }
      case 's':
      {
		//message( 1, "%s ", data );
		if ( oscMessage)
		{
		  OscMessageData* omdata = new OscMessageData( );
		  omdata->s = strdup( data );
		  omdata->omdType = OscMessageData::OmdString;
		  oscMessage->data.append( omdata );
		}
		int len = strlen( data ) + 1;
		int pad = len % 4;
		if ( pad != 0 )
			len += ( 4 - pad );
		data += len;
		count++;
		cont = true;
        break;
      }
			case 'b':
			{
			  	int blob_len = *(int*)data;  // the first int should give us the length of the blob
        		blob_len = qFromBigEndian( blob_len );	
        		if ( oscMessage)
				{
				  OscMessageData* omdata = new OscMessageData( );
				  memcpy( omdata->b, data, blob_len );
				  omdata->omdType = OscMessageData::OmdBlob;
				  oscMessage->data.append( omdata );
				}		  
				data += sizeof( int );  // step to the blob contents
				int i;
				for( i = 0; i < blob_len; i++ )
					data++;
				count++;
				cont = true;
				break;
			}
    }
  }
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

Osc::Status Osc::createOneRequest( char* buffer, int *length, char* message )
{
	char* packet = buffer;
	int size = 128; // dummy
	packet = writePaddedString( packet, &size, message );
	if( packet == NULL )
		return ERROR_CREATING_REQUEST;
		
	packet = writePaddedString( packet, &size, "," );
	if( packet == NULL )
		return ERROR_CREATING_REQUEST;
	
	*length = packet - buffer;
		
	return OK;
}

Osc::Status Osc::createMessage( OscMessage* msg )
{
	char typetag[50];
	strcpy( typetag, "," );
	for( int i = 0; i < msg->data.count( ); i++ )
	{
		OscMessageData *data = msg->data.at( i );
		if( data->omdType == OscMessageData::OmdString )
			strcat( typetag, "s" );
		if( data->omdType == OscMessageData::OmdInt )
			strcat( typetag, "i" );
		if( data->omdType == OscMessageData::OmdFloat )
			strcat( typetag, "f" );
		if( data->omdType == OscMessageData::OmdBlob )
			strcat( typetag, "b" );
	}
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

    if ( length > 0 ) // Set up to iterate through the arguments 
      bp = createMessageInternal( bp, &length, msg->address, typetag, msg->data );
    else
      bp = 0;
      
    if ( bp != 0 )
    {
      // Set the size
      *lp = qFromBigEndian( bp - mp ); 
  
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
      *lp = qFromBigEndian( bp - mp ); 
  
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
      *lp = qFromBigEndian( bp - mp ); 
  
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


char* Osc::createMessageInternal( char* bp, int* length, char* address, char* format, QList<OscMessageData*> msgData )
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
  bool cont = true;
	for( int i = 0; i < msgData.count( ); i++ )
  {
    OscMessageData *data = msgData.at(i);
		if( data->omdType == OscMessageData::OmdInt )
		{
			*length -= 4;
			if ( *length >= 0 )
			{
				int v = data->i;
				v = qFromBigEndian( v );
				*((int*)bp) = v;
				bp += 4;
			}
			else 
				cont = false;
     }
		if( data->omdType == OscMessageData::OmdFloat )
		{
			*length -= 4;
			if ( *length >= 0 )
			{
				int v;
				*((float*)&v) = data->f;
				v = qFromBigEndian( v );
				*((int*)bp) = v;
				bp += 4;
			}
			else 
				cont = false;
     }
		if( data->omdType == OscMessageData::OmdFloat )
		{
			bp = this->writePaddedString( bp, length, data->s );
			if ( bp == NULL )
				cont = false;
    }
	}

  return ( cont ) ? bp : NULL;
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
            v = qFromBigEndian( v );
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
          v = qFromBigEndian( v );
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

void Osc::uiSendPacket( QString rawString )
{
	if( !rawString.isEmpty() )
	{
		char stringBuffer[ 256 ];
		strcpy( stringBuffer, rawString.toAscii().constData() );
		createMessage( stringBuffer );
		sendPacket( );
	}
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

  *((int*)buffer) = qFromBigEndian( a );
  buffer += 4;
  *((int*)buffer) = qFromBigEndian( b );
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
	bool nullified, firstChar, gotCharacter, gotString, inQuotes;

	strcpy( typetag, "," );	//always needs to start with a comma
	
	// then we need to get the address out of the way so we can get to the arguments
	for( ip = inputString; *ip != ' ' && *ip != 0; ip++ )
		;
   
  // there may be no arguments 
  if ( *ip != 0 ) 
  {      
  	*ip = 0;	//put a null terminator in the string so we can grab the front end and stuff it in the message
  	ip++;
  	
  	bp = writePaddedString( bp, remaining, inputString );
  
  	//message( 3, "Length After Address %d\n", *remaining );
  
  	while( *ip == ' ' )
  		ip++;	//get rid of any spaces between the address and the arguments in the user's command
  		
  	// now we're at the arguments
  	char* startpoint;
  	
  	// try to pack all the parameters, don't worry about length in here, it's 
  	// handled below
  	do
  	{
  		int gotDecimals = 0;
			gotCharacter = gotString = nullified = inQuotes = false;
			firstChar = true; // flag to keep track of whether we're looking at the first character in an argument.
  		
  		// look through each character of the argument, and set the appropriate flags to tell us what kind of argument it is
  		startpoint = ip;
  		while( *ip != 0 )
  		{
  			if( *ip == '.' )
  				gotDecimals++;
  			else if( firstChar && *ip == '-' )
					{ } // do nothing, but don't register it as a character
				else
  			{
  				if( !isdigit( *ip ) )
  					gotCharacter = true;
  				if( *ip == '"' )
  				{
  					if( firstChar )
  					{
  						gotString = true;
  						inQuotes = true;
  					}
  					else
  						inQuotes = false;
  				}
  			}
  			if( *ip == ' ' && inQuotes == false )
  				break;
  			if( firstChar )
  				firstChar = false;
  			ip++;
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
  			int dummy = 1000, i;
  			if( gotString )
  			{
  				char *stringContents = startpoint + 1; // don't include the first "
  				char* endquote = stringContents; // and don't include the last one
  				for( i = 0; i < (int)strlen( stringContents ); i++ )
  				{
  					if( *endquote != '"' )
  						endquote++;
  					else
  					{
  						*endquote = '\0'; // replace the endquote with a null terminator
  						break;
  					}
  				}
  				ap = writePaddedString( ap, &dummy, stringContents );
  				gotString = false;
  			}
  			else
  				ap = writePaddedString( ap, &dummy, startpoint );
  		}
  		else if( gotDecimals == 1 )
  		{
  			strcat( typetag, "f" );
  			float floatArgument = 0.0;
  			sscanf( startpoint, "%f", &floatArgument );
  			//message( 3, "Trying to get a float from %s.  Got %f\n", startpoint, floatArgument );
        unsigned int v;
        *((float*)&v) = floatArgument;
        v = qFromBigEndian( v );
        *((int*)ap) = v;
  			ap += 4;
  		}
  		else if( strlen( startpoint ) > 0 )
  		{
  			strcat( typetag, "i" );
  			int intArgument;
  			sscanf( startpoint, "%d", &intArgument );
				intArgument = qFromBigEndian( intArgument );
  			*((int*)ap) = intArgument;
  			ap += 4;		
  		}
  		else
  		{ 
  		} //do nothing
  		firstChar = true;
  	} while( *ip != 0 || nullified );
    
    // now write the type tag and the arguments into the outgoing message buffer to create the full Osc message
  	bp = writePaddedString( bp, remaining, typetag );
  	int dataCopyLength = ap - argumentData;
  	//message( 3, "Data length %d\n", dataCopyLength );
  	// arguments are all already byte-aligned - shove them in as they are
  	memcpy( bp, argumentData, dataCopyLength );
  	*remaining -= dataCopyLength;
  	//message( 3, "Length After Data %d\n", *remaining );
  	bp+=dataCopyLength;

  }
  else
  {
    // there were no arguments, but still we need to write the address
  	bp = writePaddedString( bp, remaining, inputString );
  	// now write the blank type tag ( "," )
  	bp = writePaddedString( bp, remaining, typetag );
  }
			
	return bp;
}

void Osc::resetOutBuffer( )
{
  outBufferPointer = outBuffer;
  outBufferRemaining = OSC_MAX_MESSAGE;
  outMessageCount = 0;
}





