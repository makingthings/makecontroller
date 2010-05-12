

#ifndef OSC_H
#define OSC_H

#include "config.h"

#ifdef OSC

#include "udpsocket.h"
#include "rtos.h"
#include "osc_message.h"
#include <stdarg.h>

#define OSC_MAX_HANDLERS 56
#define OSC_MAX_MESSAGE_IN   400
#define OSC_MAX_MESSAGE_OUT  600
#define OSC_SCRATCH_BUF_SIZE 100

enum OscTransport { OscUDP, OscUSB };

class OscRangeHelper
{
public:
  OscRangeHelper( OscMessage* msg, int element, int max, int min = 0 );
  bool hasNext( );
  int next( );
private:
  int remaining, bits, current, single;
};

class OscHandler
{
public:
  int onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port )
  {
    (void)t; (void)msg; (void)src_addr; (void)src_port;
    return 0;
  }
  int onQuery( OscTransport t, char* address, int element )
  {
    (void)t; (void)address; (void)element;
    return 0;
  }
  const char* name( ) { return ""; }
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

class Osc
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
  
  static Osc* get( )
  {
    if( !_instance )
      _instance = new Osc( );
    return _instance;
  }
  
protected:
  Osc( );
  static Osc* _instance;
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
  void resetChannel( OscTransport t, bool outgoing, bool incoming );
  OscChannel* getChannel(OscTransport t);
  
  Task* autoSendTask;
  friend void oscUdpLoop( void* parameters );
  friend void oscUsbLoop( void* parameters );
  friend void oscAutoSendLoop( void* parameters );
  OscHandler* handlers[OSC_MAX_HANDLERS];
  int handler_count;

  #ifdef MAKE_CTRL_NETWORK
  Task* udpTask;
  UdpSocket udpSock;
  OscChannel udpChannel;
  int udp_listen_port, udp_reply_port, udp_reply_address;
  int udpPacketSend( char* packet, int length, int replyAddress, int replyPort );
  #endif // MAKE_CTRL_NETWORK

  #ifdef MAKE_CTRL_USB
  Task* usbTask;
  OscChannel usbChannel;
  int usbPacketSend( char* packet, int length );
  #endif // MAKE_CTRL_USB
};

#endif // OSC

#endif // OSC_H
