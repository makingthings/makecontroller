

#ifndef OSC_H
#define OSC_H

#include "types.h"
#include "ch.h"

typedef enum OscChannel_t {
  NONE,
  UDP,
  USB
} OscChannel;

typedef enum OscDataType_t {
  INT = 'i',
  FLOAT = 'f',
  STRING = 's',
  BLOB = 'b'
} OscDataType;

typedef struct OscData_t {
  OscDataType type;
  union {
    int i;
    float f;
    char* s;
    char* b;
  } value;
} OscData;

typedef void (*OscHandler)(OscChannel ch, char* address, int idx, OscData data[], int datalen);

typedef void (*OscAutosender)(OscChannel ch);

// should typically be declared const so they're located in read-only storage.
typedef struct OscNode_t {
  const char* name;
  OscHandler handler;
  uint8_t range;
  int8_t rangeOffset;
  OscAutosender autosender;
  const struct OscNode_t* children[]; // must be 0-terminated
} OscNode;

#ifdef __cplusplus
extern "C" {
#endif
bool oscUsbEnable(bool on);
bool oscUdpEnable(bool on, int port);
void oscAutosendEnable(bool enabled);
void oscUdpSetReplyPort(int port);
int  oscUdpReplyPort(void);
int  oscUdpListenPort(void);
void oscLockChannel(OscChannel ct);
void oscUnlockChannel(OscChannel ct);
bool oscCreateMessage(OscChannel ct, const char* address, OscData* data, int datacount);
int  oscSendPendingMessages(OscChannel ct);
OscChannel oscAutosendDestination(void);
void oscSetAutosendDestination(OscChannel oc);
uint32_t oscAutosendInterval(void);
void oscSetAutosendInterval(uint32_t interval);
#ifdef __cplusplus
}
#endif
#endif // OSC_H

