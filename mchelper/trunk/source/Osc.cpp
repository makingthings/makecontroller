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

OscMessage::OscMessage()
{
}

OscMessage::OscMessage(const QString & address) :
  addressPattern(address)
{

}

QString OscMessage::toString()
{
  QString msgString = this->addressPattern;
  foreach (const QVariant & d, data) {
    msgString.append(" ");
    switch (d.type()) {
      case QVariant::ByteArray:
        msgString.append("[ " + d.toByteArray().toHex() + " ]");
        break;
      default:
        msgString.append(d.toString());
        break;
    }
  }
  return msgString;
}

QByteArray OscMessage::toByteArray()
{
  QByteArray msg;
  QDataStream msgstream(&msg, QIODevice::WriteOnly);
  Osc::writePaddedString(msgstream, this->addressPattern);

  QString typetag(",");
  QByteArray args; // intermediate spot for arguments until we've assembled the typetag
  QDataStream argstream(&args, QIODevice::WriteOnly);
  foreach (const QVariant & d, data) {
    switch (d.type()) {
      case QVariant::String: {
        typetag.append('s');
        Osc::writePaddedString(argstream, d.toString());
        break;
      }
      case QVariant::ByteArray: // need to pad the blob
        typetag.append('b');
        // datastream packs this as an int32 len followed by raw data, just like OSC wants
        argstream << d.toByteArray();
        break;
      case QVariant::Int:
        typetag.append('i');
        argstream << d.toInt();
        break;
      case QMetaType::Float: // QVariant doesn't have a float type
        typetag.append('f');
        argstream << d.value<float>();
        break;
      default: break;
    }
  }
  Osc::writePaddedString(msgstream, typetag);
  msgstream.writeRawData(args.constData(), args.size());
  Q_ASSERT((msg.size() % 4) == 0);
  return msg;
}

QByteArray Osc::createPacket(const QString & msg)
{
  OscMessage oscMsg;
  if (createMessage(msg, &oscMsg))
    return oscMsg.toByteArray();
  else
    return QByteArray();
}

QByteArray Osc::createPacket(const QStringList & strings)
{
  QList<OscMessage*> oscMsgs;
  foreach (const QString & str, strings) {
    OscMessage *msg = new OscMessage();
    if (createMessage(str, msg))
      oscMsgs.append(msg);
    else
      delete msg;
  }
  QByteArray packet = createPacket(oscMsgs);
  qDeleteAll(oscMsgs);
  return packet;
}

QByteArray Osc::createPacket(const QList<OscMessage*> & msgs)
{
  QByteArray bundle;
  if (msgs.isEmpty())
    return bundle;
  else if (msgs.size() == 1) // if there's only one message in the bundle, send it as a normal message
    bundle = msgs.first()->toByteArray();
  else { // we have more than one message, and it's worth sending a real bundle
    QDataStream dstream(&bundle, QIODevice::WriteOnly);
    writePaddedString(dstream, "#bundle"); // indicate that this is indeed a bundle
    Osc::writeTimetag(dstream, 0, 0); // we don't do much with timetags

    dstream.skipRawData(bundle.size()); // make sure we're at the end of the data
    foreach (OscMessage* msg, msgs) // then write out the messages
      dstream << msg->toByteArray(); // datastream packs this as an int32 followed by raw data, just like OSC wants
  }
  Q_ASSERT((bundle.size() % 4) == 0);
  return bundle;
}

QList<OscMessage*> Osc::processPacket(const char* data, int size)
{
  QList<OscMessage*> msgList;
  QByteArray packet(data, size);
  receivePacket(&packet, &msgList);
  return msgList;
}

// When we receive a packet, check to see whether it is a message or a bundle.
bool Osc::receivePacket(QByteArray* pkt, QList<OscMessage*>* oscMessageList)
{
  QDataStream dstream(pkt, QIODevice::ReadOnly);
  if (pkt->startsWith('/')) { // single message
    OscMessage* om = receiveMessage(dstream, pkt);
    if (om)
      oscMessageList->append(om);
  }
  else if (pkt->startsWith("#bundle")) { // bundle
    dstream.skipRawData(16); // skip bundle text and timetag
    QByteArray msg;
    while (!dstream.atEnd() && dstream.status() == QDataStream::Ok) {
      dstream >> msg; // unpacks as int32 len followed by raw data, just like OSC wants
      if (msg.size() > 16384 || msg.size() <= 0) {
        qDebug() << QApplication::tr("got insane length - %1, bailing.").arg(msg.size());
        return false;
      }
      if (!receivePacket(&msg, oscMessageList))
        return false;
    }
  }
  else { // something we don't recognize...
    qDebug() << "Error - Osc packets must start with either a '/' (message) or '[' (bundle).";
  }
  return true;
}

/*
  Once we receive a message, we need to make sure it's in the right format,
  and then send it off to be interpreted (via extractData() ).
*/
OscMessage* Osc::receiveMessage(QDataStream & ds, QByteArray* msg)
{
  OscMessage* oscMessage = new OscMessage(msg->constData());
  ds.skipRawData(paddedLength(oscMessage->addressPattern));

  // We get a count back from extractData() of how many items were included - if this
  // doesn't match the length of the type tag, something funky is happening.
  if (!extractData(ds, msg, oscMessage)) {
    qDebug() << "Error extracting data from packet - type tag doesn't correspond to data included.";
    delete oscMessage;
    return 0;
  }
  return oscMessage;
}

/*
  Once we're finally in our message, we need to read the actual data.
  This means we need to step through the type tag, and then step the corresponding number of bytes
  through the data, depending on the type specified in the tag.
*/
bool Osc::extractData(QDataStream & ds, QByteArray* msg, OscMessage* oscMessage)
{
  QString typetag(msg->constData() + ds.device()->pos());
  ds.skipRawData(paddedLength(typetag));
  if (typetag.isEmpty() || !typetag.startsWith(',')) { //If there was no type tag, say so and stop processing this message.
    qDebug() << "Error - invalid type tag." << typetag;
    return false;
  }

  int typeIdx = 1; // start after the comma
  while (!ds.atEnd() && typeIdx < typetag.size()) {
    QVariant newdata;
    switch (typetag.at(typeIdx++).toAscii()) {
      case 'i': {
        int val;
        ds >> val;
        newdata.setValue(val);
        break;
      }
      case 'f': {
        float f;
        ds >> f;
        newdata.setValue(f);
        break;
      }
      case 's': {
        /*
          normally data stream wants to store int32 then string data, but
          that's not how OSC wants it so just cheat a little, and create
          a string from the QByteArray, starting at the offset that we've
          currently read to, then skip the datastream past it.
        */
        QString str(msg->constData() + ds.device()->pos());
        newdata.setValue(str);
        ds.skipRawData(paddedLength(str)); // step past any extra padding
        break;
      }
      case 'b': {
        QByteArray blob;
        ds >> blob; // data stream unpacks this as an int32 len followed by data, just like OSC wants
        newdata.setValue(blob);
        break;
      }
    }

    if (ds.status() == QDataStream::Ok && newdata.isValid()) {
      oscMessage->data.append(newdata);
    }
    else {
      qWarning() << "unhappy OSC parse..." << ds.status();
      return false;
    }
  }
  // ensure we got the appropriate number of data elements, according to the typetag
  return (oscMessage->data.size() == typetag.size() - 1);
}

void Osc::writePaddedString(QDataStream & ds, const QString & str)
{
  QByteArray paddedString = str.toAscii() + '\0'; // OSC requires that strings be null-terminated
  ds.writeRawData(paddedString.constData(), paddedString.size());
  int pad = 4 - (paddedString.size() % 4);
  if (pad < 4) { // if we had 4 - 0, that means we don't need to add any padding
    while (pad--) {
      ds << (quint8)'\0';
    }
  }
}

/*
  What is the total length of string plus padding?
*/
int Osc::paddedLength(const QString & str)
{
  int len = str.size() + 1; // size() doesn't count terminator
  int pad = len % 4;
  if (pad != 0)
    len += (4 - pad);
  return len;
}

void Osc::writeTimetag(QDataStream & ds, int a, int b)
{
  ds << a << b;
}

// we expect an address pattern followed by some number of arguments,
// delimited by spaces
bool Osc::createMessage(const QString & msg, OscMessage *oscMsg)
{
  if (!msg.startsWith("/"))
    return false;
  QStringList msgElements = msg.split(" ");
  oscMsg->addressPattern = msgElements.takeFirst();
  // now do our best to guess the type of arguments
  while (!msgElements.isEmpty()) {
    QString elmnt = msgElements.takeFirst();
    // see if it's a quoted string, presumably with spaces in it
    if (elmnt.startsWith("\"") && !elmnt.endsWith("\"")) {
      elmnt.remove(0,1); // remove opening quote
      while (!msgElements.isEmpty()) {
        elmnt += QString(" " + msgElements.takeFirst());
        if (elmnt.endsWith("\"")) {
          elmnt.remove(elmnt.size() - 1, 1); // remove the trailing quote
          break;
        }
      }
      oscMsg->data.append(QVariant(elmnt));
    }
    else { // see if it's a number and if not, assume it's a string
      bool ok = false;
      if (elmnt.count(".") == 1) { // might be a float, with exactly one .
        float f = elmnt.toFloat(&ok);
        if (ok) {
          QVariant d; // passing f directly to the constructor interprets as a double
          d.setValue(f);
          oscMsg->data.append(d);
        }
      }
      else { // it's either an int or a string
        int i = elmnt.toInt(&ok);
        if (ok)
          oscMsg->data.append(QVariant(i));
        else
          oscMsg->data.append(QVariant(elmnt)); // string
      }
    }
  }
  return true;
}

