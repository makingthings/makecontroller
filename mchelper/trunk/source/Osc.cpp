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
#include <QApplication>
#include <QtDebug>
#include <QDataStream>

QString OscMessage::toString( )
{
  QString msgString = this->addressPattern;
  foreach( QVariant d, data) {
    msgString.append( " " );
    switch( d.type() )
    {
      case QVariant::ByteArray:
        msgString.append("[ " + d.toByteArray().toHex() + " ]");
        break;
      default:
        msgString.append( d.toString() );
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
  QDataStream dstream(&args, QIODevice::WriteOnly);
  foreach( QVariant d, data ) {
    switch( d.type() )
    {
      case QVariant::String:
      {
        typetag.append( 's' );
        QByteArray ba = Osc::writePaddedString( d.toString() );
        // need to write this as raw data so it doesn't stick a length in there for us
        dstream.writeRawData(ba.constData(), ba.size());
        break;
      }
      case QVariant::ByteArray: // need to pad the blob
        typetag.append( 'b' );
        // datastream packs this as an int32 len followed by raw data, just like OSC wants
        dstream << d.toByteArray();
        break;
      case QVariant::Int:
        typetag.append( 'i' );
        dstream << d.toInt();
        break;
      default:
        // QVariant doesn't have a float type, sadly...
        if( int(d.type()) == QMetaType::type("float") ) {
            typetag.append( 'f' );
            dstream << d.value<float>();
        }
        break;
    }
  }
  msg += Osc::writePaddedString( typetag );
  msg += args;
  Q_ASSERT( ( msg.size( ) % 4 ) == 0 );
  return msg;
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
  foreach( QString str, strings ) {
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
  else { // we have more than one message, and it's worth sending a real bundle
    bundle += Osc::writePaddedString( "#bundle" ); // indicate that this is indeed a bundle
    bundle += Osc::writeTimetag( 0, 0 ); // we don't do much with timetags

    QDataStream dstream(&bundle, QIODevice::WriteOnly);
    dstream.skipRawData(bundle.size()); // make sure we're at the end of the data
    foreach(OscMessage* msg, msgs) // then write out the messages
      dstream << msg->toByteArray(); // datastream packs this as an int32 followed by raw data, just like OSC wants
  }
  Q_ASSERT( ( bundle.size( ) % 4 ) == 0 );
  return bundle;
}

QList<OscMessage*> Osc::processPacket( const char* data, int size )
{
  QList<OscMessage*> msgList;
  QByteArray packet(data, size);
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
  if(pkt->startsWith('/')) { // single message
    OscMessage* om = receiveMessage( pkt );
    if(om)
      oscMessageList->append(om);
  }
  else if(pkt->startsWith("#bundle")) { // bundle
    QDataStream dstream(pkt, QIODevice::ReadOnly);
    dstream.skipRawData(16); // skip bundle text and timetag
    QByteArray msg;
    while( !dstream.atEnd() && dstream.status() == QDataStream::Ok ) {
      dstream >> msg; // unpacks as int32 len followed by raw data, just like OSC wants
      if( msg.size() > 16384 || msg.size() <= 0 ) {
        qDebug() << QApplication::tr("got insane length - %d, bailing.").arg(msg.size());
        return false;
      }
      if( !receivePacket( &msg, oscMessageList ) )
        return false;
    }
  }
  else { // something we don't recognize...
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
  if ( typetag.isEmpty() || !typetag.startsWith(',') ) { //If there was no type tag, say so and stop processing this message.
    QString msg = QObject::tr( "Error - No type tag.");
    //messageInterface->messageThreadSafe( msg, MessageEvent::Error, preamble );
    delete oscMessage;
    return 0;
  }
  else {
    //We get a count back from extractData() of how many items were included - if this
    //doesn't match the length of the type tag, something funky is happening.
    if(!extractData( typetag, msg, oscMessage ) || oscMessage->data.size() != typetag.size() - 1) {
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
  QDataStream dstream(msg, QIODevice::ReadOnly);
  int typeIdx = 1; // start after the comma
  while(!dstream.atEnd() && typeIdx < typetag.size()) {
    QVariant newdata;
    switch( typetag.at(typeIdx++).toAscii() )
    {
      case 'i':
      {
        int val;
        dstream >> val;
        newdata.setValue(val);
        break;
      }
      case 'f':
      {
        float f;
        dstream >> f;
        newdata.setValue(f);
        break;
      }
      case 's':
      {
        /*
          normally data stream wants to store int32 then string data, but
          that's not how OSC wants it so just pull out chars until we get a null
          then skip any remaining nulls
        */
        QString str;
        quint8 c;
        while(!dstream.atEnd()) {
          dstream >> c;
          if(c == 0) {
            newdata.setValue(str);
            dstream.skipRawData(paddedLength(str) - str.size() - 1); // step past any extra padding
            break;
          }
          else
            str.append(c);
        }
        break;
      }
      case 'b':
      {
        QByteArray blob;
        dstream >> blob; // data stream unpacks this as an int32 len followed by data, just like OSC wants
        newdata.setValue(blob);
        break;
      }
    }

    if(dstream.status() == QDataStream::Ok && newdata.isValid())
      oscMessage->data.append( newdata );
    else
      return false;
  }
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
  if( pad < 4 ) { // if we had 4 - 0, that means we don't need to add any padding
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
  QDataStream dstream(&tag, QIODevice::WriteOnly);
  dstream << a << b;
  Q_ASSERT( tag.size( ) == 8 );
  return tag;
}

// we expect an address pattern followed by some number of arguments,
// delimited by spaces
bool Osc::createMessage( const QString & msg, OscMessage *oscMsg )
{
  if( !msg.startsWith( "/" ) )
    return false;
  QStringList msgElements = msg.split( " " );
  oscMsg->addressPattern = msgElements.takeFirst();
  // now do our best to guess the type of arguments
  while( !msgElements.isEmpty() ) {
    QString elmnt = msgElements.takeFirst();
    // see if it's a quoted string, presumably with spaces in it
    if( elmnt.startsWith( "\"" ) && !elmnt.endsWith("\"") ) {
      elmnt.remove(0,1); // remove opening quote
      while( !msgElements.isEmpty() ) {
        elmnt += QString(" " + msgElements.takeFirst());
        if( elmnt.endsWith( "\"" ) ) {
          elmnt.remove(elmnt.size() - 1, 1); // remove the trailing quote
          break;
        }
      }
      oscMsg->data.append( QVariant( elmnt ) );
    }
    else { // see if it's a number and if not, assume it's a string
      bool ok = false;
      if( elmnt.count( "." ) == 1) { // might be a float, with exactly one .
        float f = elmnt.toFloat( &ok );
        if( ok )
          oscMsg->data.append( QVariant( f ) );
      }
      // it's either an int or a string
      if( !ok ) {
        int i = elmnt.toInt( &ok );
        if( ok )
          oscMsg->data.append( QVariant( i ) );
        else
          oscMsg->data.append( QVariant( elmnt ) ); // string
      }
    }
  }
  return true;
}









