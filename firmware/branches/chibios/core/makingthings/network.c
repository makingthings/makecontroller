

#include "network.h"
#ifdef MAKE_CTRL_NETWORK

#include "ch.h"
#include "hal.h"

#include "lwipthread.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwipopts.h"

#include "core.h"
#include "eeprom.h"
#include "stdio.h"
#include "error.h"

#ifndef MC_DEFAULT_IP_ADDRESS
#define MC_DEFAULT_IP_ADDRESS IP_ADDRESS( 192, 168, 0, 200 )
#endif
#ifndef MC_DEFAULT_NETMASK
#define MC_DEFAULT_NETMASK    IP_ADDRESS( 255, 255, 255, 0 )
#endif
#ifndef MC_DEFAULT_GATEWAY
#define MC_DEFAULT_GATEWAY    IP_ADDRESS( 192, 168, 0, 1 )
#endif

static unsigned char macAddress[6] = {0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00};

#if (LWIP_DNS == 1)
struct Dns {
  Semaphore semaphore;
  int resolvedAddress;
};
static struct Dns dns;
static void dnsCallback(const char *name, struct ip_addr *addr, void *arg);
#endif // LWIP_DNS

static struct netif* mcnetif; // our network interface
static bool networkLastValidAddress(int* address, int *mask, int* gateway);

#if (LWIP_DHCP == 1)
static bool networkDhcpStart(void);
static bool networkDhcpStop(void);
#endif // LWIP_DHCP

void networkInit( )
{
#if (LWIP_DNS == 1)
  dns.resolvedAddress = -1;
  chSemInit(&dns.semaphore, 1);
  chSemWait(&dns.semaphore);
#endif
  
  // customize MAC address based on serial number
  int serialNumber = 123; // systemSerialNumber();
  macAddress[5] = serialNumber & 0xFF;
  macAddress[4] = ( serialNumber >> 8 ) & 0xFF;
  // Low nibble of the third byte - gives us around 1M serial numbers
  macAddress[3] = 0x50 | ( ( serialNumber >> 12 ) & 0xF );
  
  macInit(); // chibios mac init
  int address, mask, gateway;
  networkLastValidAddress(&address, &mask, &gateway);

  Semaphore initSemaphore;
  chSemInit(&initSemaphore, 1);
  chSemWait(&initSemaphore);

  lwip_socket_init();
  struct lwipthread_opts opts;
  opts.macaddress = macAddress;
  opts.address = address;
  opts.netmask = mask;
  opts.gateway = gateway;
  opts.netif = &mcnetif;
  opts.semaphore = (void*)&initSemaphore;

  chThdCreateStatic(wa_lwip_thread, LWIP_THREAD_STACK_SIZE, LOWPRIO, lwip_thread, &opts);
  chSemWait(&initSemaphore); // wait until lwip is set up
  
#if (LWIP_DHCP == 1)
  if( networkDhcp() )
    networkDhcpStart();
#endif
}

/**
  Manually set the board's IP address.
  Specify the address as the 4 numbers that make up the address - like 192.168.0.100.
  If DHCP is enabled this will have no effect, but the values will be stored and
  used the next time DHCP is disabled.
  @param a0 First element of the address.
  @param a1 Second element of the address.
  @param a2 Third element of the address.
  @param a3 Fourth element of the address.
  @return True on success, false on failure.
  
  \b Example
  \code
  Network* net = Network::get();
  net->setAddress(192, 168, 0, 100);
  \endcode
*/
bool networkSetAddress( int address, int mask, int gateway )
{
  bool rv = false;
#if (LWIP_DHCP == 1)
  if( !networkDhcp() ) { // only actually change the address if we're not using DHCP
#endif
    struct ip_addr ip, gw, netmask;
    ip.addr = address;
    netmask.addr = mask;
    gw.addr = gateway;
    netif_set_addr( mcnetif, &ip, &netmask, &gw );
#if (LWIP_DHCP == 1)
    dhcp_inform(mcnetif);
#endif
    rv = true;
#if (LWIP_DHCP == 1)
  }
#endif

  // but write the addresses to memory regardless,
  // so we can use them next time DHCP is disabled
  eepromWrite( EEPROM_SYSTEM_NET_ADDRESS, address );
  eepromWrite( EEPROM_SYSTEM_NET_MASK, mask );
  eepromWrite( EEPROM_SYSTEM_NET_GATEWAY, gateway );

  int total = address + mask + gateway;
  eepromWrite( EEPROM_SYSTEM_NET_CHECK, total );

  return rv;
}

/**
  Read the board's IP address.
  @return The board's address as an integer.  If you need to convert it
  to a string, see addressToString().
  
  \b Example
  \code
  int addr = Network::get()->address();
  // now convert it to a string
  char address[50];
  net->addressToString(address, addr);
  \endcode
*/
bool networkAddress( int* address, int* mask, int* gateway )
{
  *address = mcnetif->ip_addr.addr;
  *mask = mcnetif->netmask.addr;
  *gateway = mcnetif->gw.addr;
  return true;
}

/**
  Convert a network address into string format.
  Will result in a string of the form
  \code xxx.xxx.xxx.xxx \endcode
  @param data Where to write the string
  @param address The value, as obtained by IP_ADDRESS() or address(), mask() or gateway()
  @return The length of the address string.
  
  \b Example
  \code
  char addr[50];
  Network::addressToString(addr, IP_ADDRESS(192, 168, 0, 100));
  \endcode
*/
int networkAddressToString( char* data, int address )
{
  return sprintf( data, "%d.%d.%d.%d", 
                  IP_ADDRESS_A( address ),
                  IP_ADDRESS_B( address ),
                  IP_ADDRESS_C( address ),
                  IP_ADDRESS_D( address ));
}

#if (LWIP_DHCP == 1)

/**
  Turn DHCP on or off.
  @param enabled True to turn DHCP on, false to turn it off.
  
  \b Example
  \code
  Network* net = Network::get(); // get a reference to the Network system
  net->setDhcp(true); // turn it on
  \endcode
*/
void networkSetDhcp(bool enabled)
{
  if( enabled && !networkDhcp() ) {
    networkDhcpStart( );
    eepromWrite( EEPROM_DHCP_ENABLED, enabled );
  }
  else if( !enabled && networkDhcp() ) {
    networkDhcpStop( );
    eepromWrite( EEPROM_DHCP_ENABLED, enabled );
    int a, m, g;
    networkLastValidAddress(&a, &m, &g);
    networkSetAddress(a, m, g);
  }
}

/**
  Read whether DHCP is enabled.
  @return True if DHCP is enabled, false if not.
  
  \b Example
  \code
  Network* net = Network::get();
  if( net->dhcp() )
  {
    // then DHCP is enabled
  }
  else
  {
    // DHCP is not enabled
  }
  \endcode
*/
bool networkDhcp()
{
  return eepromRead( EEPROM_DHCP_ENABLED );
}

bool networkDhcpStart( )
{
  int count = 100;
  bool rv = false;
  if( dhcp_start( mcnetif ) != ERR_OK )
    return false;
  // now hang out for a second until we get an address
  // if DHCP is enabled but we don't find a DHCP server, just use the network config stored in EEPROM
  while( mcnetif->ip_addr.addr == 0 && count-- ) // timeout after 10 (?) seconds of waiting for a DHCP address
    chThdSleepMilliseconds( 100 );
  if( mcnetif->ip_addr.addr == 0 ) { // if we timed out getting an address via DHCP, just use whatever's in EEPROM
    int a, m, g;
    networkLastValidAddress(&a, &m, &g);
//    networkSetAddress(a, m, g);
  }
  else
    rv = true;
  return rv;
}

bool networkDhcpStop( )
{
  if(dhcp_release(mcnetif) != ERR_OK)
    return false;
  dhcp_stop(mcnetif);
  netif_set_up(mcnetif); // bring the interface back up, as dhcp_release() takes it down
  return true;
}

#endif // LWIP_DHCP

/**
  Resolve the IP address for a domain name via DNS.
  Up to 4 DNS entries are cached, so if you make successive calls to this function, 
  you won't incur a whole lookup roundtrip - you'll just get the cached value.
  The cached values are maintained internally, so if one of them becomes invalid, a
  new lookup will be fired off the next time it's asked for.
  @param name The domain to look up.
  @param timeout (optional) The number of seconds to wait for this operation to 
  complete - defaults to 30 seconds.
  @return The IP address of the host, or -1 on error.

  \b Example
  \code
  Network* net = Network::get();
  int makingthings = net->getHostByName("www.makingthings.com");
  // Now we can make a new connection to this host
  TcpSocket sock;
  if(sock.connect(makingthings, 80))
  {
    // now we can communicate
    sock.close();
  }
  \endcode
*/
#if (LWIP_DNS == 1)
int networkGetHostByName( const char *name, int timeout )
{
  struct ip_addr address;
  int retval = -1;
  err_t result = dns_gethostbyname( name, &address, dnsCallback, 0);
  if(result == ERR_OK) // the result was cached, just return it
    retval = address.addr;
  else if(result == ERR_INPROGRESS) { // a lookup is in progress - wait for the callback to signal that we've gotten a response
    if(chSemWaitTimeout(&dns.semaphore, S2ST(timeout)) == RDY_OK)
      retval = dns.resolvedAddress;
  }
  return retval;
}

/*
  The callback for a DNS look up.  The original request is waiting (via semaphore) on
  this to pop the looked up address in the right spot.
*/
void dnsCallback(const char *name, struct ip_addr *addr, void *arg)
{
  UNUSED(name);
  UNUSED(arg);
  dns.resolvedAddress = addr ? (int)addr->addr : -1;
  chSemSignal(&dns.semaphore);
}

#endif // LWIP_DNS

bool networkLastValidAddress( int* address, int *mask, int* gateway )
{
  *address  = eepromRead( EEPROM_SYSTEM_NET_ADDRESS );
  *mask     = eepromRead( EEPROM_SYSTEM_NET_MASK );
  *gateway  = eepromRead( EEPROM_SYSTEM_NET_GATEWAY );
  int total = eepromRead( EEPROM_SYSTEM_NET_CHECK );
  if ( total == *address + *mask + *gateway )
    return true;
  else {
    *address = MC_DEFAULT_IP_ADDRESS;
    *mask    = MC_DEFAULT_NETMASK;
    *gateway = MC_DEFAULT_GATEWAY;
    return false;
  }
}

#endif // MAKE_CTRL_NETWORK




