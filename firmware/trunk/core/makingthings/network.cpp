

#include "network.h"
#ifdef MAKE_CTRL_NETWORK

extern "C" {
  #include "lwip/api.h"
  #include "lwip/tcpip.h"
  #include "lwip/memp.h" 
  #include "lwip/netbuf.h" 
  #include "lwip/stats.h"
  #include "netif/loopif.h"
  #include "lwip/dhcp.h"
  #include "lwip/dns.h"
  #include "SAM7_EMAC.h"
}

#include "eeprom.h"
#include "stdio.h"
#include "error.h"

#define MC_DEFAULT_IP_ADDRESS IP_ADDRESS( 192, 168, 0, 200 )
#define MC_DEFAULT_GATEWAY    IP_ADDRESS( 192, 168, 0, 1 )
#define MC_DEFAULT_NETMASK    IP_ADDRESS( 255, 255, 255, 0 )

// MAC address definition.  The MAC address must be unique on the network.
char emacETHADDR0 = 0xAC;
char emacETHADDR1 = 0xDE;
char emacETHADDR2 = 0x48;
char emacETHADDR3 = 0x55;
char emacETHADDR4 = 0x0;
char emacETHADDR5 = 0x0;
Network* Network::_instance = 0;

void dnsCallback(const char *name, struct ip_addr *addr, void *arg);

Network::Network( )
{
  dnsResolvedAddress = -1;
  dnsSemaphore.take();
  if( !getValid() ) // if we don't have good values, set the defaults
    setDefaults( );
  else // load the values from EEPROM
  {
    Eeprom* ep = Eeprom::get();
    tempIpAddress = ep->read( EEPROM_SYSTEM_NET_ADDRESS );
    tempGateway = ep->read( EEPROM_SYSTEM_NET_GATEWAY );
    tempMask = ep->read( EEPROM_SYSTEM_NET_MASK );
  }
  
  emacETHADDR0 = 0xAC;
  emacETHADDR1 = 0xDE;
  emacETHADDR2 = 0x48;
  
  int serialNumber = 123; //System_GetSerialNumber();
  emacETHADDR5 = serialNumber & 0xFF;
  emacETHADDR4 = ( serialNumber >> 8 ) & 0xFF;
  // Low nibble of the third byte - gives us around 1M serial numbers
  emacETHADDR3 = 0x50 | ( ( serialNumber >> 12 ) & 0xF );
  
  /* Initialize lwIP and its interface layer. */
  tcpip_init( NULL, NULL ); // init all of lwip...see lwip_init() inside for the whole init story
  
  static struct netif EMAC_if;
  int address, mask, gateway;
  bool dhcp_enabled = dhcp();
  if( dhcp_enabled )
  {
    address = 0;
    mask = 0;
    gateway = 0;
  }
  else // DHCP not enabled, just read whatever the manual IP address in EEPROM is.
  {
    address = tempIpAddress;
    mask = tempMask;
    gateway = tempGateway;
  }
  // add our network interface to the system
  // Network_SetPending( 1 ); //netif_add goes away for a long time if there's no Ethernet cable connected.
  netif_add(&EMAC_if, (struct ip_addr*)&address, (struct ip_addr*)&mask, 
                        (struct ip_addr*)&gateway, NULL, ethernetif_init, tcpip_input);
  netif_set_default(&EMAC_if); // make it the default interface
  netif_set_up(&EMAC_if);      // bring it up
  EMAC_if.name[0] = 'e';       // name it so we can find it later
  EMAC_if.name[1] = 'n';
  EMAC_if.num = 0;
  
  if( dhcp_enabled )
    dhcpStart( &EMAC_if );
  
  // Network_SetPending( 0 );
}

/**
  Get a reference to the Network system.
  
  Since there's only one Network object in the system, you access it
  through get().
  @return A reference to the Network system.
  
  \b Example
  \code
  Network* net = Network::get();
  \endcode
*/
Network* Network::get( ) // static
{
  if( !_instance )
    _instance = new Network();
  return _instance;
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
bool Network::setAddress( int a0, int a1, int a2, int a3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempGateway = IP_ADDRESS( a0, a1, a2, 1 );
    tempMask = MC_DEFAULT_NETMASK;
  }
  tempIpAddress = IP_ADDRESS( a0, a1, a2, a3 );
  setValid( true ); // apply the changes and save them as valid
  return true;
}

/**
  Manually set the board's network mask.
  Specify the mask as the 4 numbers that it's composed of - like 255.255.255.0.
  If DHCP is enabled this will have no effect, but the values will be stored and
  used the next time DHCP is disabled.
  @param m0 First element of the mask.
  @param m1 Second element of the mask.
  @param m2 Third element of the mask.
  @param m3 Fourth element of the mask.
  @return True on success, false on failure.
  
  \b Example
  \code
  Network* net = Network::get();
  net->setMask(255, 255, 255, 0);
  \endcode
*/
bool Network::setMask( int m0, int m1, int m2, int m3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempGateway = MC_DEFAULT_GATEWAY;
    tempIpAddress = MC_DEFAULT_IP_ADDRESS;
  }
  tempMask = IP_ADDRESS( m0, m1, m2, m3 );
  setValid( 1 ); // apply the changes and save them as valid
  return true;
}

/**
  Manually set the board's gateway.
  Specify the gateway as the 4 numbers that it's composed of - like 192.168.0.1.
  If DHCP is enabled this will have no effect, but the values will be stored and
  used the next time DHCP is disabled.
  @param g0 First element of the gateway.
  @param g1 Second element of the gateway.
  @param g2 Third element of the gateway.
  @param g3 Fourth element of the gateway.
  @return True on success, false on failure.

  \b Example
  \code
  Network* net = Network::get();
  net->setGateway(192, 168, 0, 1);
  \endcode
*/
bool Network::setGateway( int g0, int g1, int g2, int g3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempMask = MC_DEFAULT_NETMASK;
    tempIpAddress = IP_ADDRESS( g0, g1, g2, 200 );
  }
  tempGateway = IP_ADDRESS( g0, g1, g2, g3 );
  setValid( 1 ); // apply the changes and save them as valid
  return true;
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
int Network::address( )
{
  // if( Network_GetPending() )
  //   return CONTROLLER_ERROR_NO_NETWORK;

  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  return ( mc_netif ) ? mc_netif->ip_addr.addr : -1;
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
int Network::addressToString( char* data, int address )
{
  return sprintf( data, "%d.%d.%d.%d", 
                  IP_ADDRESS_A( address ),
                  IP_ADDRESS_B( address ),
                  IP_ADDRESS_C( address ),
                  IP_ADDRESS_D( address ));
}

/**
  Read the board's network mask.
  @return The board's mask as an integer.  If you need to convert it
  to a string, see addressToString().

  \b Example
  \code
  int mask = Network::get()->mask();
  // now convert it to a string
  char maskString[50];
  net->addressToString(maskString, mask);
  \endcode
*/
int Network::mask( )
{
  // if( Network_GetPending() )
  //   return CONTROLLER_ERROR_NO_NETWORK;
  
  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  return ( mc_netif ) ? mc_netif->netmask.addr : -1;
}

/**
  Read the board's gateway.
  @return The board's gateway as an integer.  If you need to convert it
  to a string, see addressToString().

  \b Example
  \code
  int gateway = Network::get()->gateway();
  // now convert it to a string
  char gateString[50];
  net->addressToString(gateString, gateway);
  \endcode
*/
int Network::gateway( )
{
  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  return ( mc_netif ) ? mc_netif->gw.addr : -1;
}

/**
  Turn DHCP on or off.
  @param enabled True to turn DHCP on, false to turn it off.
  
  \b Example
  \code
  Network* net = Network::get(); // get a reference to the Network system
  net->setDhcp(true); // turn it on
  \endcode
*/
void Network::setDhcp(bool enabled)
{
  if( enabled && !dhcp() )
  {
    // we specify our network interface as en0 when we init
    struct netif* mc_netif = netif_find( (char*)"en0" );
    if( mc_netif != NULL )
      dhcpStart( mc_netif );
      
    Eeprom::get()->write( EEPROM_DHCP_ENABLED, enabled );
  }
  
  if( !enabled && dhcp() )
  {
    // we specify our network interface as en0 when we init
    struct netif* mc_netif = netif_find( (char*)"en0" );
    if( mc_netif != NULL )
      dhcpStop( mc_netif );
    Eeprom::get()->write( EEPROM_DHCP_ENABLED, enabled );
    setValid( 1 );
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
bool Network::dhcp()
{
  return (Eeprom::get()->read( EEPROM_DHCP_ENABLED ) == 1);
}

void Network::dhcpStart( struct netif* netif )
{
  // Network_SetPending( 1 ); // set a flag so nobody else tries to set up this netif
  int count = 0;
  dhcp_start( netif );
  // now hang out for a second until we get an address
  // if DHCP is enabled but we don't find a DHCP server, just use the network config stored in EEPROM
  while( netif->ip_addr.addr == 0 && count++ < 100 ) // timeout after 10 (?) seconds of waiting for a DHCP address
    Task::sleep( 100 );
  if( netif->ip_addr.addr == 0 ) // if we timed out getting an address via DHCP, just use whatever's in EEPROM
  {
    struct ip_addr ip, gw, mask; // network config stored in EEPROM
    ip.addr = tempIpAddress;
    mask.addr = tempMask;
    gw.addr = tempGateway;
    netif_set_addr( netif, &ip, &mask, &gw );
  }
  // Network_SetPending( 0 );
}

void Network::dhcpStop( struct netif* netif )
{
  dhcp_release( netif );
  netif_set_up(netif); // bring the interface back up, as dhcp_release() takes it down
  return;
}

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
int Network::getHostByName( const char *name, int timeout )
{
  struct ip_addr addr;
  int retval = -1;
  err_t result = dns_gethostbyname( name, &addr, dnsCallback, this);
  if(result == ERR_OK) // the result was cached, just return it
    retval = addr.addr;
  else if(result == ERR_INPROGRESS) // a lookup is in progress - wait for the callback to signal that we've gotten a response
  {
    if(dnsSemaphore.take(timeout * 1000))
      retval = dnsResolvedAddress;
  }
  return retval;
}

/*
  The callback for a DNS look up.  The original request is waiting (via semaphore) on
  this to pop the looked up address in the right spot.
*/
void dnsCallback(const char *name, struct ip_addr *addr, void *arg)
{
  LWIP_UNUSED_ARG(name);
  Network* network = (Network*)arg;
  network->dnsResolvedAddress = (addr != NULL) ? addr->addr : -1;
  network->dnsSemaphore.give();
}

int Network::setValid( bool v )
{
  if ( v )
  {
    struct ip_addr ip, gw, mask;
    ip.addr = tempIpAddress; // these should each have been set previously
    mask.addr = tempMask;  // by Network_SetAddress(), etc.
    gw.addr = tempGateway;
    if( !dhcp() ) // only actually change the address if we're not using DHCP
    {
      // we specify our network interface as en0 when we init
      struct netif* mc_netif = netif_find( (char*)"en0" );
      if( mc_netif != NULL )
        netif_set_addr( mc_netif, &ip, &mask, &gw );
    }
    
    // but write the addresses to memory regardless, so we can use them next time we boot up without DHCP
    Eeprom* ep = Eeprom::get();
    ep->write( EEPROM_SYSTEM_NET_ADDRESS, ip.addr );
    ep->write( EEPROM_SYSTEM_NET_MASK, mask.addr );
    ep->write( EEPROM_SYSTEM_NET_GATEWAY, gw.addr );

    int total = tempIpAddress + tempMask + tempGateway;
    ep->write( EEPROM_SYSTEM_NET_CHECK, total );

    // Network_Valid = NET_VALID;
  }
  else
    Eeprom::get()->write( EEPROM_SYSTEM_NET_CHECK, 0 );

  return CONTROLLER_OK;
}

bool Network::getValid( )
{
  Eeprom* ep = Eeprom::get();
  int address = ep->read( EEPROM_SYSTEM_NET_ADDRESS );
  int mask = ep->read( EEPROM_SYSTEM_NET_MASK );
  int gateway = ep->read( EEPROM_SYSTEM_NET_GATEWAY );
  int total = ep->read( EEPROM_SYSTEM_NET_CHECK );
  return ( total == address + mask + gateway );
}

void Network::setDefaults( )
{
  tempIpAddress = MC_DEFAULT_IP_ADDRESS;
  tempGateway = MC_DEFAULT_GATEWAY;
  tempMask = MC_DEFAULT_NETMASK;
  setValid( 1 );
}

#endif // MAKE_CTRL_NETWORK




