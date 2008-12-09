

#ifndef OSC_CPP_H
#define OSC_CPP_H

#include "udpsocket.h"
#include "rtos_.h"
#include "osc_message.h"
#include <stdarg.h>

#define OSC_MAX_HANDLERS 56
#define OSC_MAX_MESSAGE_IN   400
#define OSC_MAX_MESSAGE_OUT  600
#define OSC_SCRATCH_BUF_SIZE 100

#define Oscc OSCC::instance()

enum OscTransport { oscUDP, oscUSB };

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
  virtual int onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port ) = 0;
  virtual int onQuery( OscTransport t, char* address, int element ) = 0;
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
  int outgoingMsgCount;
  OscMessage incomingMsg;
  Semaphore outgoingSemaphore;
  int (*sendMessage)( char* packet, int length, int replyAddress, int replyPort );
};

class OSCC
{
public:
  void setUsbListener( bool enable );
  void setUdpListener( bool enable, int port = 0 );
  void setAutoSender( bool enable );
  bool registerHandler( OscHandler* handler );
  int pendingMessages( OscTransport t );
  int send( OscTransport t );
  int createMessage( OscTransport t, const char* address, const char* format, ... );
  static int endianSwap( int a );
  
  static OSCC* instance( )
  {
    if( !_instance )
      _instance = new OSCC( );
    return _instance;
  }
  
protected:
  OSCC( );
  static OSCC* _instance;
  bool receivePacket( OscTransport t, char* packet, int length );
  int receiveMessage( OscTransport t, char* message, int length );
  int handleQuery( OscTransport t, char* message );
  OscMessage* extractData( OscTransport t, char* message, int length );
  char* createMessageInternal( char* bp, int* length, const char* address, const char* format, va_list args );
  char* createBundle( char* buffer, int* length, int a, int b );
  char* findTypeTag( char* message, int length );
  char* writePaddedString( char* buffer, int* length, const char* string );
  char* writePaddedBlob( char* buffer, int* length, char* blob, int blen );
  char* writeTimetag( char* buffer, int* length, int a, int b );
  int sendInternal( OscTransport t );
  int usbPacketSend( char* packet, int length );
  void resetChannel( OscTransport t, bool outgoing, bool incoming );
  
  Task* usbTask;
  Task* autoSendTask;
  friend void oscUdpLoop( void* parameters );
  friend void oscUsbLoop( void* parameters );
  friend void oscAutoSendLoop( void* parameters );
  OscHandler* handlers[OSC_MAX_HANDLERS];
  int handler_count;
  #ifdef MAKE_CTRL_NETWORK
  Task* udpTask;
  UdpSocket send_sock;
  OscChannel udpChannel;
  int udp_listen_port, udp_reply_port, udp_reply_address;
  int udpPacketSend( char* packet, int length, int replyAddress, int replyPort );
  #endif // MAKE_CTRL_NETWORK
  OscChannel usbChannel;
  OscChannel* getChannel(OscTransport t);
  
};

#endif // OSC_CPP_H
