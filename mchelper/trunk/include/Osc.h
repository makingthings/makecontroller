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

#ifndef OSC_H
#define OSC_H

#include <QString>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QVariant>

class OscMessage
{
public:
  QString addressPattern;
  QList<QVariant> data;
  QString toString( );
  QByteArray toByteArray( );
};

class Osc
{
  public:
    Osc() {}
    static QByteArray writePaddedString(const QString & str);
    static QByteArray writeTimetag(int a, int b);
    static QByteArray createOneRequest(const char* message);
    QList<OscMessage*> processPacket(const char* data, int size);
    QByteArray createPacket(const QStringList & strings);
    QByteArray createPacket(const QList<OscMessage*> & msgs);
    QByteArray createPacket(const QString & msg);
    bool createMessage(const QString & msg, OscMessage *oscMsg);

  private:
    QString getTypeTag(QByteArray* msg);
    bool receivePacket(QByteArray* pkt,  QList<OscMessage*>* oscMessageList);
    OscMessage* receiveMessage(QByteArray* msg);
    bool extractData(const QString & typetag, QByteArray* buffer, OscMessage* message);
    int paddedLength(const QString & str);
};

#endif


