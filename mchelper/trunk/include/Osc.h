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

class OscData
{
public:
  enum { String, Int, Float, Blob } type;
  OscData( int i );
  OscData( float f );
  OscData( QString s );
  OscData( QByteArray b );

  QString s() { return data.toString(); }
  QByteArray b() { return data.toByteArray(); }
  int i() { return data.toInt(); }
  float f() { return data.toDouble(); }

protected:
  QVariant data;
};

class OscMessage
{
public:
  QString addressPattern;
  QList<OscData*> data;
  QString toString( );
  QByteArray toByteArray( );
  ~OscMessage( ) { qDeleteAll( data ); }
};

class Osc
{
  public:
    Osc( ) { };
    static QByteArray writePaddedString( const char *string );
    static QByteArray writePaddedString( QString str );
    static QByteArray writeTimetag( int a, int b );
    static QByteArray createOneRequest( const char* message );
    QList<OscMessage*> processPacket( char* data, int size );
    QByteArray createPacket( QStringList strings );
    QByteArray createPacket( QList<OscMessage*> msgs );
    QByteArray createPacket( QString msg );

    void setPreamble( QString preamble ) { this->preamble = preamble; }
    QString getPreamble( );
    bool createMessage( QString msg, OscMessage *oscMsg );

  private:
    char* findDataTag( char* message, int length );
    QString getTypeTag( char* message );
    bool receivePacket( char* packet, int length,  QList<OscMessage*>* oscMessageList );
    void receiveMessage( char* message, int length, QList<OscMessage*>* oscMessageList );
    int extractData( char* buffer, OscMessage* message );
    QString preamble;
};

#endif


