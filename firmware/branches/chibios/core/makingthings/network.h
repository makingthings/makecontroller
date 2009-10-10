

#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"
#ifdef MAKE_CTRL_NETWORK
#include "types.h"

extern char macAddress[];

/**
  \def IP_ADDRESS( a, b, c, d )
  Generate an address appropriate for Socket functions from 4 integers.
  \b Example
  \code
  void* sock = Socket( IP_ADDRESS( 192, 168, 0, 200 ), 80 );
  \endcode
*/
#define IP_ADDRESS( a, b, c, d ) ( ( (int)d << 24 ) + ( (int)c << 16 ) + ( (int)b << 8 ) + (int)a )
#define IP_ADDRESS_D( address )  ( ( (int)address >> 24 ) & 0xFF )
#define IP_ADDRESS_C( address )  ( ( (int)address >> 16 ) & 0xFF ) 
#define IP_ADDRESS_B( address )  ( ( (int)address >>  8 ) & 0xFF )
#define IP_ADDRESS_A( address )  ( ( (int)address       ) & 0xFF )

/**
  The network system on the Controller.
  
  The Make Controller has a full network system with DHCP, DNS, sockets, and a simple web server.
  The Network class manages the network configuration of the board - its address, gateway, etc.
  
  \section Usage
  To get started using the Network system, grab a reference to the central Network object - since
  there's only one, you don't create your own.  Then you can set the address, check the address, 
  manage DHCP and more.
  
  \code
  Network* net = Network::get(); // get a reference to the Network object
  int a = net->address(); // check our address
  net->setDhcp(true); // turn on dhcp
  \endcode
  
  \section DHCP
  DHCP is a mechanism that allows the board to automatically get an IP address from a router.  If 
  you plug your board onto a network that has a router (this is quite common), enabling DHCP is 
  the simplest way to get set up - it's highly recommended - use setDhcp() and dhcp().  If you 
  have a situation that requires a manual address, turn DHCP off and set it yourself with setAddress().
  
  \section DNS
  DNS is a mechanism that resolves a domain name, like www.makingthings.com, into the IP address
  of the MakingThings web site.  This is the same thing that happens in your browser when you type
  an address in.  The Make Controller can do this as well - check getHostByName().
  
  For a general overview of the network capabilities, check out the \ref networking page.
  
  \ingroup networking
*/

void networkInit(void);
bool networkSetAddress(int address, int mask, int gateway);
bool networkAddress(int* address, int* mask, int* gateway);
int  networkGetHostByName(const char *name, int timeout);
void networkSetDhcp(bool enabled);
bool networkDhcp(void);
int  networkAddressToString(char* data, int address);

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
#endif // NETWORK_H
