/*********************************************************************************

 Copyright 2006-2009 MakingThings

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
#include <QApplication>
#include <QtDebug>
#include <QBuffer>

QString OscMessage::toString( )
{
  QString msgString = this->addressPattern;
  foreach( OscData* dataElement, data)
  {
    msgString.append( " " );
    switch( dataElement->type )
    {
      case OscData::Blob:
        msgString.append("[ " + dataElement->b().toHex() + " ]");
        break;
      case OscData::String:
      case OscData::Int:
      case OscData::Float:
        msgString.append( dataElement->s() );
        break;
    }
  }
  return msgString;
}

QByteArray OscMessage::toByteArray( )
{
  QByteArray msg = Osc::writePaddedString( this->addressPattern );
  QString typetag( "," );
  QByteArray args; // intermediate spot for arguments until we've assembled the typetag
  foreach( OscData* dataElement, data )
  {
    switch( dataElement->type )
    {
      case OscData::String:
        typetag.append( 's' );
        args.append( Osc::writePaddedString( dataElement->s() ) );
        break;
      case OscData::Blob: // need to pad the blob
        typetag.append( 'b' );
        args.append( Osc::writePaddedString( dataElement->s() ) );
        break;
      case OscData::Int:
      {
        typetag.append( 'i' );
        QByteArray intarg(sizeof(int), 0);
        *(int*)intarg.data() = qToBigEndian( dataElement->i() );
        args.append( intarg );
        break;
      }
      case OscData::Float:
      {
        typetag.append( 'f' );
        QByteArray floatarg(sizeof(int), 0);
        *(int*)floatarg.data() = qToBigEndian( (int)dataElement->f() );
        args.append( floatarg );
        break;
      }
    }
  }
  msg += Osc::writePaddedString( typetag );
  msg += args;

  Q_ASSERT( ( msg.size( ) % 4 ) == 0 );
  return msg;
}

OscData::OscData( int i )
{
  type = Int;
  data.setValue(i);
}
OscData::OscData( float f )
{
  type = Float;
  data.setValue(f);
}
OscData::OscData( const QString & s )
{
  type = String;
  data.setValue(s);
}
OscData::OscData( const QByteArray & b )
{
  type = Blob;
  data.setValue(b);
}

QByteArray Osc::createPacket( const QString & msg )
{
  OscMessage oscMsg;
  if( createMessage( msg, &oscMsg ) )
    return oscMsg.toByteArray( );
  else
    return QByteArray( );
}

QByteArray Osc::createPacket( const QStringList & strings )
{
  QList<OscMessage*> oscMsgs;
  foreach( QString str, strings )
  {
    OscMessage *msg = new OscMessage( );
    if( createMessage( str, msg ) )
      oscMsgs.append( msg );
    else
      delete msg;
  }
  QByteArray packet = createPacket( oscMsgs );
  qDeleteAll( oscMsgs );
  return packet;
}

QByteArray Osc::createPacket( const QList<OscMessage*> & msgs )
{
  QByteArray bundle;
  if( msgs.size( ) == 0 )
    return bundle;
  else if( msgs.size( ) == 1 ) // if there's only one message in the bundle, send it as a normal message
    bundle = msgs.first()->toByteArray( );
  else // we have more than one message, and it's worth sending a real bundle
  {
    bundle += Osc::writePaddedString( "#bundle" ); // indicate that this is indeed a bundle
    bundle += Osc::writeTimetag( 0, 0 ); // we don't do much with timetags
    foreach(OscMessage* msg, msgs) // then write out the messages
    {
      QByteArray m = msg->toByteArray();
      QByteArray bundlesize(sizeof(int), 0);
      *(int*)bundlesize.data() = qToBigEndian( m.size() );
      bundle += bundlesize; // each message in a bundle is preceded by its int32 size
      bundle += m;
    }
  }
  Q_ASSERT( ( bundle.size( ) % 4 ) == 0 );
  return bundle;
}

QList<OscMessage*> Osc::processPacket( char* data, int size )
{
  QList<OscMessage*> msgList;
  QByteArray packet = QByteArray(data, size);
  receivePacket( &packet, &msgList );
  return msgList;
}

/*
  An OSC Type Tag String is an OSC-string beginning with the character ',' (comma) followed by a sequence
  of characters corresponding exactly to the sequence of OSC Arguments in the given message.
  Each character after the comma is called an OSC Type Tag and represents the type of the corresponding OSC Argument.
  (The requirement for OSC Type Tag Strings to start with a comma makes it easier for the recipient of an OSC Message
  to determine whether that OSC Message is lacking an OSC Type Tag String.)

  OSC Type Tag  Type of corresponding argument
      i         int32
      f         float32
      s         Osc-string
      b         Osc-blob
*/
QString Osc::getTypeTag( QByteArray* msg )
{
  // get rid of everything up until the beginning of the typetag, which is indicated by ','
  while(msg->at(0) != ',' && msg->size())
    msg->remove(0, 1);
  QString tag( msg->data() );
  msg->remove(0, paddedLength(tag)); // remove the tag and trailing nulls from the buffer
  return tag;
}

// When we receive a packet, check to see whether it is a message or a bundle.
bool Osc::receivePacket( QByteArray* pkt, QList<OscMessage*>* oscMessageList )
{
  if(pkt->startsWith('/')) // single message
  {
    OscMessage* om = receiveMessage( pkt );
    if(om)
      oscMessageList->append(om);
  }
  else if(pkt->startsWith("#bundle")) // bundle
  {
    pkt->remove(0, 16); // skip bundle text and timetag
    while( pkt->size() )
    {
      if( pkt->size() > 16384 || pkt->size() <= 0 )
      {
        qDebug() << QApplication::tr("got insane length - %d, bailing.").arg(pkt->size());
        return false;
      }
      int messageLength = qFromBigEndian( *(int*)pkt->data() );
      pkt->remove(0, sizeof(int));
      if ( messageLength <= pkt->size() )
      {
        QByteArray msg = pkt->left(messageLength);
        if( !receivePacket( &msg, oscMessageList ) )
          return false;
      }
      pkt->remove(0, messageLength);
    }
  }
  else // something we don't recognize...
  {
    QString msg = QObject::tr( "Error - Osc packets must start with either a '/' (message) or '[' (bundle).");
    //messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
  }
  return true;
}

/*
  Once we receive a message, we need to make sure it's in the right format,
  and then send it off to be interpreted (via extractData() ).
*/
OscMessage* Osc::receiveMessage( QByteArray* msg )
{
  OscMessage* oscMessage = new OscMessage( );
  oscMessage->addressPattern = QString( msg->data() );
  msg->remove(0, paddedLength(oscMessage->addressPattern));

  QString typetag = getTypeTag( msg );
  if ( typetag.isEmpty() || !typetag.startsWith(',') ) //If there was no type tag, say so and stop processing this message.
  {
    QString msg = QObject::tr( "Error - No type tag.");
    //messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
    delete oscMessage;
    return 0;
  }
  else
  {
    //We get a count back from extractData() of how many items were included - if this
    //doesn't match the length of the type tag, something funky is happening.
    if(!extractData( typetag, msg, oscMessage ) || oscMessage->data.size() != typetag.size() - 1)
    {
      QString msg = QObject::tr( "Error extracting data from packet - type tag doesn't correspond to data included.");
      //messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
      delete oscMessage;
      return 0;
    }
    else
      return oscMessage;
  }
}

/*
  Once we're finally in our message, we need to read the actual data.
  This means we need to step through the type tag, and then step the corresponding number of bytes
  through the data, depending on the type specified in the tag.
*/
bool Osc::extractData( const QString & typetag, QByteArray* msg, OscMessage* oscMessage )
{
  QBuffer buf(msg);
  buf.open(QIODevice::ReadOnly);
  int typeIdx = 1; // start after the comma
  while(!buf.atEnd() && typeIdx < typetag.size())
  {
    switch( typetag.at(typeIdx++).toAscii() )
    {
      case 'i':
      {
        QByteArray i(sizeof(int), 0);
        if(buf.read(i.data(), i.size()) < 1)
          return false;
        int val = qFromBigEndian( *(int*)i.data() );
        oscMessage->data.append( new OscData((val)) );
        break;
      }
      case 'f':
      {
        QByteArray i(sizeof(int), 0);
        if(buf.read(i.data(), i.size()) < 1)
          return false;
        int val = qFromBigEndian( *(int*)i.data() );
        float f = *(float*)&val;
        oscMessage->data.append( new OscData(f) );
        break;
      }
      case 's':
      {
        QString str(buf.buffer().mid(buf.pos()).data());
        oscMessage->data.append( new OscData(str) );
        buf.read(paddedLength(str)); // pull it out of the buffer
        break;
      }
      case 'b':
      {
        QByteArray blob(sizeof(int), 0); // first read the int32 blob length
        if(buf.read(blob.data(), blob.size()) < 1)
          return false;
        int blob_len = qFromBigEndian( *(int*)blob.data() );
        blob.resize(blob_len);
        if(buf.read(blob.data(), blob_len) < 1) // then get the blob data
          return false;
        oscMessage->data.append( new OscData( blob ) );
        break;
      }
    }
  }
  buf.close();
  return true;
}

QByteArray Osc::createOneRequest( const char* message )
{
  QByteArray oneRequest = Osc::writePaddedString( message );
  oneRequest += Osc::writePaddedString( "," );
  Q_ASSERT( ( oneRequest.size( ) % 4 ) == 0 );
  return oneRequest;
}

QByteArray Osc::writePaddedString( const QString & str )
{
  QByteArray paddedString = str.toAscii() + '\0'; // OSC requires that strings be null-terminated
  int pad = 4 - ( paddedString.size( ) % 4 );
  if( pad < 4 ) // if we had 4 - 0, that means we don't need to add any padding
  {
    while(pad--)
      paddedString.append( '\0' );
  }

  Q_ASSERT( ( ( paddedString.size( ) ) % 4 ) == 0 );
  return paddedString;
}

/*
  What is the total length of string plus padding?
*/
int Osc::paddedLength( const QString & str )
{
  int len = str.size() + 1; // size() doesn't count terminator
  int pad = len % 4;
  if ( pad != 0 )
    len += ( 4 - pad );
  return len;
}

QByteArray Osc::writeTimetag( int a, int b )
{
  QByteArray tag;
  QByteArray bytes(sizeof(int), 0);
  *(int*)bytes.data() = qToBigEndian( a );
  tag += bytes;

  *(int*)bytes.data() = qToBigEndian( b );
  tag += bytes;

  Q_ASSERT( tag.size( ) == 8 );
  return tag;
}

// we expect an address pattern followed by some number of arguments,
// delimited by spaces
bool Osc::createMessage( const QString & msg, OscMessage *oscMsg )
{
  QStringList msgElements = msg.split( " " );
  if( !msgElements.at( 0 ).startsWith( "/" ) )
    return false;

  oscMsg->addressPattern = msgElements.takeFirst();

  // now do our best to guess the type of arguments
  while( msgElements.size( ) )
  {
    QString elmnt = msgElements.takeFirst();
    if( elmnt.startsWith( "\"" ) ) // see if it's a quoted string, presumably with spaces in it
    {
      if( !elmnt.endsWith( "\"" ) )
      {
        // we got a quote...zip through successive elements and find a matching end quote
        while( msgElements.size( ) )
        {
          if( msgElements.first().endsWith( "\"" ) )
          {
            elmnt += QString( " %1" ).arg( msgElements.takeFirst() );
            break;
          }
          else
            elmnt += msgElements.takeFirst();
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









