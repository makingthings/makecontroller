

#ifndef NETWORK__H
#define NETWORK__H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK
#include "rtos_.h"

/**
  The network system on the Controller.  
  \ingroup Network
*/
class Network
{
public:
  Network( );
  int setAddress( int a0, int a1, int a2, int a3 );
  int setMask( int m0, int m1, int m2, int m3 );
  int setGateway( int g0, int g1, int g2, int g3 );
  int getAddress( );
  int getMask( );
  int getGateway( );
  int getHostByName( const char *name );
  void setDhcp(bool enabled);
  bool getDhcp();
  
protected:
  void setDefaults();
  int setValid( int v );
  bool getValid( );
  Semaphore dnsSemaphore;
  int dnsResolvedAddress;
  friend void dnsCallback(const char *name, struct ip_addr *addr, void *arg);
  int tempIpAddress, tempGateway, tempMask;
  void dhcpStart( struct netif* netif );
  void dhcpStop( struct netif* netif );
  char emacETHADDR0, emacETHADDR1, emacETHADDR2;
  char emacETHADDR3, emacETHADDR4, emacETHADDR5;
};

#ifdef OSC
#include "osc_cpp.h"

class NetworkOSC : public OscHandler
{
public:
  NetworkOSC( ) { }
  int onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port );
  int onQuery( OscTransport t, char* address, int element );
  const char* name( ) { return "network"; }
  static const char* propertyList[];
  
  int getUdpListenPort( );
  int getUdpSendPort( );
  
protected:
  int udp_listen_port;
  int udp_send_port;
};

#endif // OSC
#endif // MAKE_CTRL_NETWORK
#endif // NETWORK__H
