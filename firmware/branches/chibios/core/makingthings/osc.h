

#ifndef OSC_H
#define OSC_H

#include "types.h"
#include "ch.h"

typedef enum OscChannel_t {
  UDP,
  USB
} OscChannel;

typedef enum OscDataType_t {
  INT,
  FLOAT,
  STRING,
  BLOB
} OscDataType;

typedef struct OscData_t {
  OscDataType type;
  union {
    int i;
    float f;
    char* s;
    char* b;
  };
} OscData;

typedef bool (*OscHandler)(OscChannel ch, char* address, short idx, OscData data[], int datalen);

// childNodes members are flexible length arrays.
// should ideally be declared const so
// they're located in read-only storage.
typedef struct OscNode_t {
  const char* name;
  OscHandler handler;
  uint8_t indexCount;
  uint8_t indexOffset;
  const struct OscNode_t* children[]; // must be 0-terminated
} OscNode;

#ifdef __cplusplus
extern "C" {
#endif
bool oscUsbEnable(bool on);
bool oscUdpEnable(bool on, int port);
int  oscSplitAddress(char* buf, char* elems[], int maxelems);
bool oscCreateMessage(OscChannel ct, const char* address, OscData* data, int datacount);
int  oscSendPendingMessages(OscChannel ct);
#ifdef __cplusplus
}
#endif
#endif // OSC_H

