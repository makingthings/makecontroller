

#include "config.h"
#ifdef MAKE_CTRL_NETWORK

#include "network.h"
#include "core.h"

#include "lwipthread.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwipopts.h"

#include "stdio.h"
#include "error.h"

#ifndef MC_DEFAULT_IP_ADDRESS
#define MC_DEFAULT_IP_ADDRESS IP_ADDRESS(192, 168, 0, 200)
#endif
#ifndef MC_DEFAULT_NETMASK
#define MC_DEFAULT_NETMASK    IP_ADDRESS(255, 255, 255, 0)
#endif
#ifndef MC_DEFAULT_GATEWAY
#define MC_DEFAULT_GATEWAY    IP_ADDRESS(192, 168, 0, 1)
#endif

static uint8_t macAddress[6] = {0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00};

#if LWIP_DNS
struct Dns {
  Semaphore semaphore;
  int resolvedAddress;
};
static struct Dns dns;
static void dnsCallback(const char *name, struct ip_addr *addr, void *arg);
#endif // LWIP_DNS

static struct netif* mcnetif; // our network interface
static bool networkLastValidAddress(int* address, int *mask, int* gateway);

#if LWIP_DHCP
static SEMAPHORE_DECL(dhcpSem, 0);
static void lwipStatusCallback(struct netif *netif);
static bool networkDhcpStart(int timeout);
static bool networkDhcpStop(int timeout);
#endif // LWIP_DHCP

/**
  \defgroup Network Network
  Control the network system on the Controller.

  The Make Controller has a full network system with DHCP, DNS, sockets, and a simple web server.
  The Network class manages the network configuration of the board - its address, gateway, etc.

  \section Usage
  First, turn on the network system via networkInit().  You can then manually set the address,
  manage DHCP, etc.

  \section DHCP
  DHCP is a mechanism that allows the board to automatically get an IP address from a router.  If
  you plug your board onto a network that has a router (this is quite common), enabling DHCP is
  the simplest way to get set up - it's highly recommended - use networkSetDhcp() and networkDhcp().  If you
  have a situation that requires a manual address, turn DHCP off and set it yourself with networkSetAddress().

  \section DNS
  DNS is a mechanism that resolves a domain name, like www.makingthings.com, into the IP address
  of the MakingThings web site.  This is the same thing that happens in your browser when you type
  an address in.  The Make Controller can do this as well - check getHostByName().

  For a general overview of the network capabilities, check out the \ref networking page.

  \ingroup networking
  @{
*/

/**
  Turn on the network system.
*/
void networkInit()
{
  // customize MAC address based on serial number
  int serialNumber = 123; // systemSerialNumber();
  macAddress[5] = serialNumber & 0xFF;
  macAddress[4] = (serialNumber >> 8) & 0xFF;
  // Low nibble of the third byte - gives us around 1M serial numbers
  macAddress[3] = 0x50 | ((serialNumber >> 12) & 0xF);
  
  macInit(); // chibios mac init
  int address, mask, gateway;
  // if DHCP is compiled in and it's enabled, init address values with 0 - DHCP doesn't seem to work otherwise
#if LWIP_DHCP
  bool dhcp = networkDhcp();
  if (dhcp)
    address = mask = gateway = 0;
  else
    networkLastValidAddress(&address, &mask, &gateway);
#else
    networkLastValidAddress(&address, &mask, &gateway);
#endif // LWIP_DHCP

  Semaphore initSemaphore;
  chSemInit(&initSemaphore, 0);

  lwip_socket_init();
  struct lwipthread_opts opts;
  opts.macaddress = macAddress;
  opts.address = address;
  opts.netmask = mask;
  opts.gateway = gateway;
  opts.netif = &mcnetif;
  opts.semaphore = (void*)&initSemaphore;

  chThdCreateStatic(wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 1, lwip_thread, &opts);
  chSemWait(&initSemaphore); // wait until lwip is set up

  mcnetif->hostname = "tester";
#if LWIP_DHCP
  mcnetif->status_callback = lwipStatusCallback;
  if (dhcp)
    networkDhcpStart(5000);
#endif
}

/**
  Manually set the network address.
  The network address consists of the IP address, mask and gateway.

  If DHCP is enabled this will have no effect, but the values will be stored and
  used the next time DHCP is disabled.
  @param address The IP address you want to use.
  @param mask The network mask to use - 255.255.255.0 is quite common
  @param gateway The gateway address - often, this is the same as the address, with the last digit changed to 1.
  @return True on success, false on failure.
  
  \b Example
  \code
  networkSetAddress(IP_ADDRESS(192, 168, 0, 100), IP_ADDRESS(255, 255, 255, 0), IP_ADDRESS(192, 168, 0, 1));
  \endcode
*/
bool networkSetAddress(int address, int mask, int gateway)
{
  bool rv = false;
#if LWIP_DHCP
  if (!networkDhcp()) { // only actually change the address if we're not using DHCP
#endif
    struct ip_addr ip, gw, netmask;
    ip.addr = address;
    netmask.addr = mask;
    gw.addr = gateway;
    netif_set_addr(mcnetif, &ip, &netmask, &gw);
    rv = true;
#if LWIP_DHCP
  }
#endif

  // but write the addresses to memory regardless,
  // so we can use them next time DHCP is disabled
  eepromWrite(EEPROM_SYSTEM_NET_ADDRESS, address);
  eepromWrite(EEPROM_SYSTEM_NET_MASK, mask);
  eepromWrite(EEPROM_SYSTEM_NET_GATEWAY, gateway);

  int total = address + mask + gateway;
  eepromWrite(EEPROM_SYSTEM_NET_CHECK, total);

  return rv;
}

/**
  Read the board's IP address.
  @return The board's address as an integer.  If you need to convert it
  to a string, see addressToString().
  
  \b Example
  \code
  int address;
  networkAddress(&address, 0, 0);
  // now convert it to a string
  char buf[50];
  networkAddressToString(buf, address);
  \endcode
*/
void networkAddress(int* address, int* mask, int* gateway)
{
  if (address) *address = mcnetif->ip_addr.addr;
  if (mask)    *mask = mcnetif->netmask.addr;
  if (gateway) *gateway = mcnetif->gw.addr;
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
  int len = networkAddressToString(addr, IP_ADDRESS(192, 168, 0, 100));
  \endcode
*/
int networkAddressToString(char* data, int address)
{
  return siprintf(data, "%d.%d.%d.%d",
                  IP_ADDRESS_A(address),
                  IP_ADDRESS_B(address),
                  IP_ADDRESS_C(address),
                  IP_ADDRESS_D(address));
}

/**
  Convert a string representation of a network address into an integer.
  The input string should be in the form \code xxx.xxx.xxx.xxx \endcode.
  @param str The string to convert.
  @return The address as an integer, or -1 on failure.
*/
int networkAddressFromString(char *str)
{
  int a1, a2, a3, a4;
  if (siscanf(str, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4)
    return IP_ADDRESS(a1, a2, a3, a4);
  else
    return -1;
}

#if LWIP_DHCP

void lwipStatusCallback(struct netif *netif)
{
  if (netif == mcnetif)
    chSemSignal(&dhcpSem);
}

/**
  Turn DHCP on or off.
  @param enabled True to turn DHCP on, false to turn it off.
  
  \b Example
  \code
  networkSetDhcp(ON, 0); // turn it on, but don't wait around for a new address
  \endcode
*/
void networkSetDhcp(bool enabled, int timeout)
{
  if (enabled && !networkDhcp()) {
    networkDhcpStart(timeout);
    eepromWrite(EEPROM_DHCP_ENABLED, enabled);
  }
  else if(!enabled && networkDhcp()) {
    networkDhcpStop(timeout);
    eepromWrite(EEPROM_DHCP_ENABLED, enabled);
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
  if (networkDhcp() == ON) {
    // then DHCP is enabled
  }
  else {
    // DHCP is not enabled
  }
  \endcode
*/
bool networkDhcp()
{
  return eepromRead(EEPROM_DHCP_ENABLED);
}

bool networkDhcpStart(int timeout)
{
  netif_set_down(mcnetif); // note - dhcp_start brings it back up
  if (netifapi_dhcp_start(mcnetif) != ERR_OK)
    return false;
  // now hang out for a second until we get an address
  return chSemWaitTimeout(&dhcpSem, MS2ST(timeout)) == RDY_OK;
}

bool networkDhcpStop(int timeout)
{
  netifapi_dhcp_stop(mcnetif);
  bool rv = (chSemWaitTimeout(&dhcpSem, MS2ST(timeout)) == RDY_OK);
  netifapi_netif_set_up(mcnetif); // bring the interface back up, as dhcp_release() takes it down
  return rv;
}

#endif // LWIP_DHCP

/**
  Resolve the IP address for a domain name via DNS.
  Up to 4 DNS entries are cached, so if you make successive calls to this function, 
  you won't incur a whole lookup roundtrip - you'll just get the cached value.
  The cached values are maintained internally, so if one of them becomes invalid, a
  new lookup will be fired off the next time it's asked for.
  @param name The domain to look up.
  @param timeout The number of seconds to wait for this operation to complete.
  @return The IP address of the host, or -1 on error.

  \b Example
  \code
  int makingthings = networkGetHostByName("www.makingthings.com");
  // Now we can make a new connection to this host
  int socket = tcpOpen(makingthings, 80);
  if (socket > -1) {
    // now we can communicate
    tcpClose(socket);
  }
  \endcode
*/
#if LWIP_DNS
int networkGetHostByName(const char *name, int timeout)
{
  struct ip_addr address;
  err_t result = dns_gethostbyname(name, &address, dnsCallback, &dns);
  if (result == ERR_OK) { // the result was cached, just return it
    return address.addr;
  }
  else if (result == ERR_INPROGRESS) {
    // a lookup is in progress - wait for the callback to signal that we've gotten a response
    return (chSemWaitTimeout(&dns.semaphore, S2ST(timeout)) == RDY_OK) ? dns.resolvedAddress : -1;
  }
  else
    return -1;
}

/** @} */

/*
  The callback for a DNS look up.  The original request is waiting (via semaphore) on
  this to pop the looked up address in the right spot.
*/
void dnsCallback(const char *name, struct ip_addr *addr, void *arg)
{
  UNUSED(name);
  struct Dns* dns = arg;
  dns->resolvedAddress = addr ? (int)addr->addr : -1;
  chSemSignal(&dns->semaphore);
}

#endif // LWIP_DNS

bool networkLastValidAddress(int* address, int *mask, int* gateway)
{
  int total = eepromRead(EEPROM_SYSTEM_NET_CHECK);
  *address  = eepromRead(EEPROM_SYSTEM_NET_ADDRESS);
  *mask     = eepromRead(EEPROM_SYSTEM_NET_MASK);
  *gateway  = eepromRead(EEPROM_SYSTEM_NET_GATEWAY);
  if (total == *address + *mask + *gateway) {
    return true;
  }
  else {
    *address = MC_DEFAULT_IP_ADDRESS;
    *mask    = MC_DEFAULT_NETMASK;
    *gateway = MC_DEFAULT_GATEWAY;
    return false;
  }
}

#ifdef OSC

static void networkOscFindHandler(OscChannel ch, char* address, int idx, OscData data[], int datalen)
{
  UNUSED(idx); UNUSED(datalen); UNUSED(data);

  char addrbuf[16];
  int a;
  networkAddress(&a, 0 , 0);
  networkAddressToString(addrbuf, a);
  OscData d[4] = {
    { .type = STRING, .value.s = addrbuf },
    { .type = INT,    .value.i = 10000 },
    { .type = INT,    .value.i = 10000 },
    { .type = STRING, .value.s = "myname" }
  };
  oscCreateMessage(ch, address, d, 4);
}

static void networkOscDhcpHandler(OscChannel ch, char* address, int idx, OscData data[], int datalen)
{
  UNUSED(idx);
  if (datalen == 0) { // it's a request
    OscData d;
    d.value.i = networkDhcp() ? 1 : 0;
    d.type = INT;
    oscCreateMessage(ch, address, &d, 1);
  }
  else if (datalen == 1 && data[0].type == INT) {
    networkSetDhcp(data[0].value.i, 0);
  }
}

static void networkOscAddressHandler(OscChannel ch, char* address, int idx, OscData data[], int datalen)
{
  UNUSED(idx);
  if (datalen == 3 && data[0].type == STRING && data[1].type == STRING && data[2].type == STRING) {
    // set our address
    int a = networkAddressFromString(data[0].value.s);
    int m = networkAddressFromString(data[1].value.s);
    int g = networkAddressFromString(data[2].value.s);
    networkSetAddress(a, m, g);
  }
  else if (datalen == 0) {
    char addrbuf[16];
    char maskbuf[16];
    char gatewaybuf[16];
    int a, m, g;
    networkAddress(&a, &m, &g);
    networkAddressToString(addrbuf, a);
    networkAddressToString(maskbuf, m);
    networkAddressToString(gatewaybuf, g);
    OscData d[3] = {
      { .type = STRING, .value.s = addrbuf },
      { .type = STRING, .value.s = maskbuf },
      { .type = STRING, .value.s = gatewaybuf }
    };
    oscCreateMessage(ch, address, d, 3);
  }
}

static void networkOscUdpPortHandler(OscChannel ch, char* address, int idx, OscData data[], int datalen)
{
  UNUSED(idx);
  if (datalen == 0) { // it's a request
    OscData d = { .value.i = oscUdpReplyPort(), .type = INT };
    oscCreateMessage(ch, address, &d, 1);
  }
  else if (datalen == 1 && data[0].type == INT) {
    oscUdpSetReplyPort(data[0].value.i);
  }
}

static const OscNode networkOscFind = { .name = "find", .handler = networkOscFindHandler };
static const OscNode networkOscDhcp = { .name = "dhcp", .handler = networkOscDhcpHandler };
static const OscNode networkOscAddress = { .name = "address", .handler = networkOscAddressHandler };
static const OscNode networkOscUdpPort = { .name = "osc_udp_listen_port", .handler = networkOscUdpPortHandler };

const OscNode networkOsc = {
  .name = "network",
  .children = {
    &networkOscFind,
    &networkOscDhcp,
    &networkOscAddress,
    &networkOscUdpPort, 0
  }
};

#endif // OSC
#endif // MAKE_CTRL_NETWORK
