

#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

/**
  \def IP_ADDRESS(a, b, c, d)
  Generate an address appropriate for Socket functions from 4 integers.
  \b Example
  \code
  void* sock = Socket( IP_ADDRESS( 192, 168, 0, 200 ), 80 );
  \endcode
*/
#define IP_ADDRESS(a, b, c, d) (((int)d << 24) + ((int)c << 16) + ((int)b << 8) + (int)a)
#define IP_ADDRESS_D(address)  (((int)address >> 24) & 0xFF)
#define IP_ADDRESS_C(address)  (((int)address >> 16) & 0xFF) 
#define IP_ADDRESS_B(address)  (((int)address >>  8) & 0xFF)
#define IP_ADDRESS_A(address)  (((int)address      ) & 0xFF)
#define IP_ADDRESS_BROADCAST 0xffffffffUL
#define IP_ADDRESS_ANY       0x00000000UL

#ifdef __cplusplus
extern "C" {
#endif
void networkInit(void);
bool networkSetAddress(int address, int mask, int gateway);
void networkAddress(int* address, int* mask, int* gateway);
int  networkGetHostByName(const char *name);
void networkSetDhcp(bool enabled, int timeout);
bool networkDhcp(void);
int  networkAddressToString(char* data, int address);
int  networkAddressFromString(char *str);
#ifdef __cplusplus
}
#endif

#ifdef OSC
#include "osc.h"
extern const OscNode networkOsc;
#endif // OSC
#endif // NETWORK_H
