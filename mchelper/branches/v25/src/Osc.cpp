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

#include "Osc.h"
#include <QtCore/qendian.h>
#include <QObject>

QString OscMessage::toString( )
{
	QString msgString = this->addressPattern;
	for( int j = 0; j < data.size( ); j++ )
	{
		msgString.append( " " );
		OscData *dataElement = data.at( j );
		switch( dataElement->type )
		{
			case OscData::OscBlob:
			{
				unsigned char* blob = (unsigned char*)dataElement->b.data();
				if( blob == NULL )
				{
          int blob_len = qFromBigEndian( *(int*)blob );  // the first int should give us the length of the blob
					blob += sizeof( int ); // step past the blob_len
          
					QString blobString( "[ " );
					while( blob_len-- )
						blobString.append( "%1 " ).arg( QString::number( *blob++, 16  ) );
					blobString.append( "]" );
					
	        msgString.append( blobString );
				}
				else
					msgString.append( "[ ]" );
				break;
			}
			case OscData::OscString:
				msgString.append( dataElement->s );
				break;
			case OscData::OscInt:
				msgString.append( QString::number( dataElement->i ) );
				break;
			case OscData::OscFloat:
				msgString.append( QString::number( dataElement->f) );
				break;
		}
	}
	return msgString;
}

QByteArray OscMessage::toByteArray( )
{
	QByteArray msg;
	msg += Osc::writePaddedString( this->addressPattern );
	QString typetag( "," );
	QList<QByteArray> args; // intermediate spot for arguments until we've assembled the typetag
	for( int i = 0; i < data.size( ); i++ )
	{
		switch( data.at(i)->type )
		{
			case OscData::OscString:
				typetag.append( 's' );
				args.append( Osc::writePaddedString( data.at(i)->s ) );
				break;
			case OscData::OscBlob: // need to pad the blob
				typetag.append( 'b' );
				args.append( Osc::writePaddedString( data.at(i)->s ) );
				break;
			case OscData::OscInt:
			{
				typetag.append( 'i' );
				QByteArray intarg;
				intarg.resize( sizeof( int ) );
				*(int*)intarg.data() = qToBigEndian( data.at(i)->i );
				args.append( intarg );
				break;
			}
			case OscData::OscFloat:
			{
				typetag.append( 'f' );
				QByteArray floatarg;
				floatarg.resize( sizeof( int ) );
				*(int*)floatarg.data() = qToBigEndian( (int)data.at(i)->f );
				args.append( floatarg );
				break;
			}
		}
	}
	msg += Osc::writePaddedString( typetag );
	for( int i = 0; i < args.size(); i++ )
		msg += args.at( i );
	Q_ASSERT( ( msg.size( ) % 4 ) == 0 );
	return msg;
}

OscData::OscData( int i )
{
	type = OscInt;
	this->i = i;
}
OscData::OscData( float f )
{
	type = OscFloat;
	this->f = f;
}
OscData::OscData( QString s )
{
	type = OscString;
	this->s = s;
}
OscData::OscData( QByteArray b )
{
	type = OscBlob;
	this->b = b;
}

QByteArray Osc::createPacket( QString msg )
{
	OscMessage oscMsg;
	if( createMessage( msg, &oscMsg ) )
		return oscMsg.toByteArray( );
	else
		return QByteArray( );
}

QByteArray Osc::createPacket( QStringList strings )
{
	QList<OscMessage*> oscMsgs;
	for( int i = 0; i < strings.size( ); i++ )
	{
		OscMessage *msg = new OscMessage( );
		if( createMessage( strings.at( i ), msg ) )
			oscMsgs.append( msg );
		else
			delete msg;
	}
	QByteArray packet = createPacket( oscMsgs );
	qDeleteAll( oscMsgs );
	return packet;
}

QByteArray Osc::createPacket( QList<OscMessage*> msgs )
{
	QByteArray bundle;
	if( msgs.size( ) == 0 )
		return bundle;
	else if( msgs.size( ) == 1 ) // if there's only one message in the bundle, send it as a normal message
		bundle = msgs.at( 0 )->toByteArray( );
	else // we have more than one message, and it's worth sending a real bundle
	{
		bundle += Osc::writePaddedString( "#bundle" ); // indicate that this is indeed a bundle
		bundle += Osc::writeTimetag( 0, 0 ); // we don't do much with timetags
		for( int i = 0; i < msgs.count( ); i++ ) // then write out the messages
		{
			QByteArray msg = msgs.at(i)->toByteArray( );
			QByteArray msgSize;
			msgSize.resize( sizeof( int ) );
			*(int*)msgSize.data() = qToBigEndian( msg.size() );
			bundle += msgSize; // each message in a bundle is preceded by its int32 size
			bundle += msg;
		}
	}
	Q_ASSERT( ( bundle.size( ) % 4 ) == 0 );
	return bundle;
}

QString Osc::getPreamble( )
{ 
	return preamble;
}

QList<OscMessage*> Osc::processPacket( char* data, int size )
{
	QList<OscMessage*> msgList;
	receivePacket( data, size, &msgList );
	return msgList;
}

//void Osc::setInterfaces( MessageInterface* messageInterface )
//{
//  this->messageInterface = messageInterface;
//}

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

QString Osc::getTypeTag( char *message )
{
	QString tag( message );
	int tagIndex = tag.indexOf( ',' );
	if( tagIndex < 0 )
		return QString( );
	else
	{
		tag = tag.remove( 0, tagIndex );
		return tag;
	}
}

// When we receive a packet, check to see whether it is a message or a bundle.
bool Osc::receivePacket( char* packet, int length, QList<OscMessage*>* oscMessageList )
{
	//qDebug( "Raw: %s, Length: %d\n", packet, length );
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
          if( length > 16384 || length <= 0 )
          {
            qDebug("got insane length - %d, bailing.", length);
            return false;
          }
          // read the length (pretend packet is a pointer to integer)
          int messageLength = qFromBigEndian( *((int*)packet) );
          packet += 4;
          length -= 4;
          if ( messageLength <= length )
          {
            if( !receivePacket( packet, messageLength, oscMessageList ) )
              return false;
          }
          length -= messageLength;
          packet += messageLength;
        }
      }
      break;
		default:
			// something we don't recognize...
			QString msg = QObject::tr( "Error - Osc packets must start with either a '/' (message) or '[' (bundle).");
			//messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
	}
  return true;
}

/*
	Once we receive a message, we need to make sure it's in the right format,
	and then send it off to be interpreted (via extractData() ).
*/
void Osc::receiveMessage( char* in, int length, QList<OscMessage*>* oscMessageList )
{
	OscMessage* oscMessage = new OscMessage( );
	oscMessage->addressPattern = QString( in );
	
	// QString typetag = getTypeTag( in );
	// typetagLength = Osc::writePaddedString( typetag ).size( );
	
	// Then try to find the type tag
	char* type = findDataTag( in, length );
  if ( type == NULL )		//If there was no type tag, say so and stop processing this message.
  {
		QString msg = QObject::tr( "Error - No type tag.");
		//messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
		delete oscMessage;
  }
	else		//Otherwise, step through the type tag and print the data out accordingly.
	{
		//We get a count back from extractData() of how many items were included - if this
		//doesn't match the length of the type tag, something funky is happening.
		int count = extractData( type, oscMessage );
		if ( count != (int)( strlen(type) - 1 ) )
		{
			QString msg = QObject::tr( "Error extracting data from packet - type tag doesn't correspond to data included.");
			//messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
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
				data += 4;
				count++;
				if ( oscMessage )
				{
					OscData* omdata = new OscData( );
					omdata->i = i;
					omdata->type = OscData::OscInt;
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
					oscMessage->data.append( new OscData( f ) );

				data += 4;
				count++;
				cont = true;
        break;
      }
      case 's':
      {
				if ( oscMessage)
					oscMessage->data.append( new OscData( QString( data ) ) );

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
				// the first int should give us the length of the blob, but also account for the blob_len itself
				int	blob_len = qFromBigEndian( *(int*)data ) + 4; 
				if ( oscMessage)
					oscMessage->data.append( new OscData( QByteArray::fromRawData( data, blob_len ) ) );  
				data += blob_len;
				count++;
				cont = true;
				break;
			}
    }
  }
  return count;
}

QByteArray Osc::createOneRequest( char* message )
{
	QByteArray oneRequest;
	oneRequest += Osc::writePaddedString( message );
	oneRequest += Osc::writePaddedString( "," );
	Q_ASSERT( ( oneRequest.size( ) % 4 ) == 0 );
	return oneRequest;
}

QByteArray Osc::writePaddedString( QString str )
{
	return writePaddedString( str.toAscii().data() );
}

QByteArray Osc::writePaddedString( char *string )
{
	QByteArray paddedString( string );
	paddedString.append( '\0' ); // OSC requires that strings be null-terminated
  int pad = 4 - ( paddedString.size( ) % 4 );
	if( pad < 4 ) // if we had 4 - 0, that means we don't need to add any padding
	{
		for( int i = 0; i < pad; i++ )
			paddedString.append( '\0' );
	}
		
	Q_ASSERT( ( ( paddedString.size( ) ) % 4 ) == 0 );
	return paddedString;
}

QByteArray Osc::writeTimetag( int a, int b )
{
	QByteArray tag;
	QByteArray bits;
	
	bits.resize( sizeof( int ) );
	*(int*)bits.data() = qToBigEndian( a );
	tag += bits;
	
	*(int*)bits.data() = qToBigEndian( b );
	tag += bits;
	Q_ASSERT( tag.size( ) == 8 );
	return tag;
}

// we expect an address pattern followed by some number of arguments, 
// delimited by spaces
bool Osc::createMessage( QString msg, OscMessage *oscMsg )
{
	QStringList msgElements = msg.split( " " );
	if( !msgElements.at( 0 ).startsWith( "/" ) )
		return false;

	oscMsg->addressPattern = msgElements.takeAt( 0 );
	
	// now do our best to guess the type of arguments
	int count = 0;
	while( count < msgElements.size( ) )
	{
		QString elmnt = msgElements.at( count++ );
		if( elmnt.startsWith( "\"" ) ) // see if it's a quoted string, presumably with spaces in it
		{
			if( !elmnt.endsWith( "\"" ) )
			{
				// we got a quote...zip through successive elements and find a matching end quote
				while( count < msgElements.size( ) )
				{
					if( msgElements.at( count ).endsWith( "\"" ) )
					{
						elmnt += QString( " %1" ).arg( msgElements.at( count++ ) );
						break;
					}
					else
						elmnt += msgElements.at( count++ );
				}
			}
			oscMsg->data.append( new OscData( elmnt.remove( "\"" ) ) ); // TODO, only remove first and last quotes
		}
		else if( elmnt.startsWith( "-" ) ) // see if it's a negative number
		{
			bool ok;
			if( elmnt.contains( "." ) )
			{
				float f = elmnt.toFloat( &ok );
				if( ok )
					oscMsg->data.append( new OscData( f ) );
			}
			else
			{
				int i = elmnt.toInt( &ok );
				if( ok )
					oscMsg->data.append( new OscData( i ) );
			}
			
		}
		else // no more special cases.  see if it's a number and if not, assume it's a string
		{
			bool ok = false;
			if( elmnt.count( "." ) == 1) // might be a float, with exactly one .
			{
				float f = elmnt.toFloat( &ok );
				if( ok )
					oscMsg->data.append( new OscData( f ) );
			}
			// it's either an int or a string
			if( !ok )
			{
				int i = elmnt.toInt( &ok );
				if( ok )
					oscMsg->data.append( new OscData( i ) );
				else
					oscMsg->data.append( new OscData( elmnt ) ); // a string
			}
		}
	}
	return true;
}









