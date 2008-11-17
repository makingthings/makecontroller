

#ifndef OSC_CPP_H
#define OSC_CPP_H

#include "udpsocket.h"

#define OSC_MAX_HANDLERS 56
#define OSC_MSG_MAX_DATA_ITEMS 20
#define OSC_MAX_MESSAGE_IN   400
#define OSC_MAX_MESSAGE_OUT  600
#define OSC_SCRATCH_BUF_SIZE 100

#define mOSC OSC::instance()

enum OscTransport { UDP, USB };
enum OscDataType { Int, Float, String, Blob };

typedef struct OscData
{
  union
  {
    int i;
    float f;
    char* s;
  };
  OscDataType type;
};

class OscMessage
{
public:
  char* address;
  OscData data_items[OSC_MSG_MAX_DATA_ITEMS];
  int data_count;
  
  int   addressElementAsInt( int element, bool* ok = 0 );
  float addressElementAsFloat( int element, bool* ok = 0 );
  char* addressElementAsString( int element );
  
  int   dataItemAsInt( int index, bool* ok = 0 );
  float dataItemAsFloat( int index, bool* ok = 0 );
  char* dataItemAsString( int index );
  char* dataItemAsBlob( int index, int* blob_len );
};

class OscRangeHelper
{
public:
  OscRangeHelper( OscMessage* msg, int element, int max, int min = 0 );
  bool hasNextIndex( );
  int nextIndex( );
private:
  int remaining, bits, current, single;
};

class OscHandler
{
public:
  // mandatory
  virtual int onNewMsg( OscMessage* msg, OscTransport t, int src_addr, int src_port ) = 0;
  virtual int onQuery( int element ) = 0;
  virtual const char* name( ) = 0;
  // optional
  bool autoSend( OscTransport t ) { (void)t; return false; }
protected:
  int propertyLookup( const char* propertyList[], char* property );
};

typedef struct OscChannel
{
  char inBuf[OSC_MAX_MESSAGE_IN];
  char outBuf[OSC_MAX_MESSAGE_OUT];
  char scratchBuf[OSC_SCRATCH_BUF_SIZE];
  char* outBufPtr;
  int outBufRemaining;
  OscMessage incomingMsg;
  xSemaphoreHandle semaphore;
  int (*sendMessage)( char* packet, int length, int replyAddress, int replyPort );
};

class OSCC
{
public:
  void setUsbListener( bool enable );
  void setUdpListener( bool enable, int port = 0 );
  void setAutoSender( bool enable );
  bool registerHandler( const char* address, OscHandler* handler );
  int pendingMessages( OscTransport t );
  int sendMessages( OscTransport t );
  int createMessage( OscTransport t, char* address, char* format, ... );
  static void* getAddressElement( int position );
  OSCC* instance( )
  {
    if( !_instance )
      _instance = new OSCC( );
    return _instance;
  }
  
protected:
  OSCC( ) { }
  OSCC* _instance;
  bool receivePacket( OscTransport t, char* packet, int length );
  char* writePaddedString( char* buffer, int* length, char* string );
  char* writePaddedBlob( char* buffer, int* length, char* blob, int blen );
  char* writeTimetag( char* buffer, int* length, int a, int b );
  int endianSwap( int a );
  void udpTask( void* parameters );
  void usbTask( void* parameters );
  void asyncTask( void* parameters );
  int udpPacketSend( char* packet, int length, int replyAddress, int replyPort );
  int usbPacketSend( char* packet, int length );
  
  void* udpTaskPtr;
  void* usbTaskPtr;
  void* asyncTaskPtr;
  OscHandler* handlers[OSC_MAX_HANDLERS];
  UdpSocket send_sock;
  OscChannel usbChannel;
  OscChannel udpChannel;
};

#endif // OSC_CPP_H
