

#include "network_.h"
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

#include "eeprom_.h"

void dnsCallback(const char *name, struct ip_addr *addr, void *arg);

Network::Network( )
{
  dnsResolvedAddress = -1;
  dnsSemaphore.take();
  if( !getValid() ) // if we don't have good values, set the defaults
    setDefaults( );
  else // load the values from EEPROM
  {
    tempIpAddress = EEPROM->read( EEPROM_SYSTEM_NET_ADDRESS );
    tempGateway = EEPROM->read( EEPROM_SYSTEM_NET_GATEWAY );
    tempMask = EEPROM->read( EEPROM_SYSTEM_NET_MASK );
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
  bool dhcp = true ; //getDhcp();
  if( dhcp )
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
  
  if( dhcp )
    dhcpStart( &EMAC_if );
  
  // Network_SetPending( 0 );
}

int Network::setAddress( int a0, int a1, int a2, int a3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempGateway = NETIF_IP_ADDRESS( a0, a1, a2, 1 );
    tempMask = NETIF_IP_ADDRESS( 255, 255, 255, 0 );
  }
  tempIpAddress = NETIF_IP_ADDRESS( a0, a1, a2, a3 );
  setValid( 1 ); // apply the changes and save them as valid
  return CONTROLLER_OK;
}

int Network::setMask( int m0, int m1, int m2, int m3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempGateway = NETIF_IP_ADDRESS( 192, 168, 0, 1 );
    tempIpAddress = NETIF_IP_ADDRESS( 192, 168, 0, 200 );
  }
  tempMask = NETIF_IP_ADDRESS( m0, m1, m2, m3 );
  setValid( 1 ); // apply the changes and save them as valid
  return CONTROLLER_OK;
}

int Network::setGateway( int g0, int g1, int g2, int g3 )
{
  if( !getValid() ) // make sure the other elements are initialized, set to defaults
  {
    tempMask = NETIF_IP_ADDRESS( 255, 255, 255, 255 );
    tempIpAddress = NETIF_IP_ADDRESS( g0, g1, g2, 200 );
  }
  tempGateway = NETIF_IP_ADDRESS( g0, g1, g2, g3 );
  setValid( 1 ); // apply the changes and save them as valid
  
  return CONTROLLER_OK;
}

int Network::getAddress( )
{
  // if( Network_GetPending() )
  //   return CONTROLLER_ERROR_NO_NETWORK;

  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  if( mc_netif != NULL )
    return mc_netif->ip_addr.addr;
  else // if the Ethernet interface is not up, we'll just get garbage back
    return -1;
}

int Network::getMask( )
{
  // if( Network_GetPending() )
  //   return CONTROLLER_ERROR_NO_NETWORK;
  
  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  if( mc_netif != NULL )
    return mc_netif->netmask.addr;
  else // if the Ethernet interface is not up, we'll just get garbage back
    return -1;
}

int Network::getGateway( )
{
  // we specify our network interface as en0 when we init
  struct netif* mc_netif = netif_find( (char*)"en0" );
  if( mc_netif != NULL )
    return mc_netif->gw.addr;
  else // if the Ethernet interface is not up, we'll just get garbage back
    return -1;
}

void Network::setDhcp(bool enabled)
{
  if( enabled && !getDhcp() )
  {
    struct netif* mc_netif;
    // we specify our network interface as en0 when we init
    mc_netif = netif_find( (char*)"en0" );
    if( mc_netif != NULL )
      dhcpStart( mc_netif );
      
    EEPROM->write( EEPROM_DHCP_ENABLED, enabled );
  }
  
  if( !enabled && getDhcp() )
  {
    struct netif* mc_netif;
    // we specify our network interface as en0 when we init
    mc_netif = netif_find( (char*)"en0" );
    if( mc_netif != NULL )
      dhcpStop( mc_netif );
    EEPROM->write( EEPROM_DHCP_ENABLED, enabled );
    setValid( 1 );
  }
}

bool Network::getDhcp()
{
  int state = EEPROM->read( EEPROM_DHCP_ENABLED );
  return (state == 1) ? 1 : 0;
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

int Network::getHostByName( const char *name )
{
  struct ip_addr addr;
  int retval = -1;
  err_t result = dns_gethostbyname( name, &addr, dnsCallback, this);
  if(result == ERR_OK) // the result was cached, just return it
    retval = addr.addr;
  else if(result == ERR_INPROGRESS) // a lookup is in progress - wait for the callback to signal that we've gotten a response
  {
    if(dnsSemaphore.take(30000)) // timeout is 30 seconds by default
      retval = dnsResolvedAddress;
  }
  return ntohl(retval);
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

int Network::setValid( int v )
{
  if ( v )
  {
    struct ip_addr ip, gw, mask;
    struct netif* mc_netif;

    ip.addr = tempIpAddress; // these should each have been set previously
    mask.addr = tempMask;  // by Network_SetAddress(), etc.
    gw.addr = tempGateway;
    if( !getDhcp() ) // only actually change the address if we're not using DHCP
    {
      // we specify our network interface as en0 when we init
      mc_netif = netif_find( (char*)"en0" );
      if( mc_netif != NULL )
        netif_set_addr( mc_netif, &ip, &mask, &gw );
    }
    
    // but write the addresses to memory regardless, so we can use them next time we boot up without DHCP
    EEPROM->write( EEPROM_SYSTEM_NET_ADDRESS, ip.addr );
    EEPROM->write( EEPROM_SYSTEM_NET_MASK, mask.addr );
    EEPROM->write( EEPROM_SYSTEM_NET_GATEWAY, gw.addr );

    int total = tempIpAddress + tempMask + tempGateway;
    EEPROM->write( EEPROM_SYSTEM_NET_CHECK, total );

    // Network_Valid = NET_VALID;
  }
  else
  {
    // int value = 0;
    EEPROM->write( EEPROM_SYSTEM_NET_CHECK, 0 );
    // Network_Valid = NET_INVALID;
  }

  return CONTROLLER_OK;
}

bool Network::getValid( )
{
  int address = EEPROM->read( EEPROM_SYSTEM_NET_ADDRESS );
  int mask = EEPROM->read( EEPROM_SYSTEM_NET_MASK );
  int gateway = EEPROM->read( EEPROM_SYSTEM_NET_GATEWAY );
  int total = EEPROM->read( EEPROM_SYSTEM_NET_CHECK );
  return ( total == address + mask + gateway ) ? true : false;
}

// if things aren't valid, just set the defaults
void Network::setDefaults( )
{
  tempIpAddress = NETIF_IP_ADDRESS( 192, 168, 0, 200 );
  tempGateway = NETIF_IP_ADDRESS( 192, 168, 0, 1 );
  tempMask = NETIF_IP_ADDRESS( 255, 255, 255, 0 );
  setValid( 1 );
}

#endif // MAKE_CTRL_NETWORK




