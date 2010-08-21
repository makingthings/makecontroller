


#include "core.h"
#ifdef OSC

#include "osc.h"
#include "osc_patternmatch.h"
#include "osc_data.h"
#include <string.h>
#include <stdio.h>

#ifndef OSC_MAX_MSG_IN
#define OSC_MAX_MSG_IN 512
#endif

#ifndef OSC_MAX_MSG_OUT
#define OSC_MAX_MSG_OUT 512
#endif

#ifndef OSC_MAX_DATA_ITEMS
#define OSC_MAX_DATA_ITEMS 20
#endif

#ifndef OSC_AUTOSEND_STACK_SIZE
#define OSC_AUTOSEND_STACK_SIZE 512
#endif

#ifndef OSC_UDP_DEFAULT_PORT
#define OSC_UDP_DEFAULT_PORT 10000
#endif

#ifndef OSC_AUTOSEND_MAX_INTERVAL
#define OSC_AUTOSEND_MAX_INTERVAL 5000
#endif

#ifndef OSC_AUTOSEND_DEFAULT_INTERVAL
#define OSC_AUTOSEND_DEFAULT_INTERVAL 10
#endif

typedef int (*OscSendMsg)(const char* data, int len);

typedef struct OscChannelData_t {
  Mutex lock;
  uint8_t outMsgCount;
  uint32_t outBufRemaining;
  char* outBufPtr;
  char outBuf[OSC_MAX_MSG_OUT];
  char inBuf[OSC_MAX_MSG_IN];
  OscSendMsg sendMessage;
} OscChannelData;

typedef struct Osc_t {
#ifdef MAKE_CTRL_USB
  Thread* usbThd;
  OscChannelData usb;
#endif
#ifdef MAKE_CTRL_NETWORK
  Thread* udpThd;
  OscChannelData udp;
  int udpsock;
  int udpReplyPort;
  int udpReplyAddress;
  int udpListenPort;
#endif
  Thread* autosendThd;
  OscChannel autosendDestination;
  uint32_t autosendPeriod;
} Osc;

static void oscReceivePacket(OscChannel ch, char* data, uint32_t len);
static void oscReceiveMessage(OscChannel ch, char* data, uint32_t len);
static void oscResetChannel(OscChannelData* ch);
static OscChannelData* oscGetChannelByType(OscChannel ct);
static uint32_t oscExtractData(char* buf, uint32_t len, OscData data[], int maxdata);
static bool oscDispatchNode(OscChannel ch, char* addr, char* fulladdr,
                              const OscNode* node, OscData d[], int datalen);
static bool oscNameSpaceQuery(OscChannel ch, char* addr, char *fulladdr, const OscNode* node);

static Osc osc;
extern const OscNode oscRoot; // must be defined by the user

#ifdef MAKE_CTRL_USB

#ifndef OSC_USB_STACK_SIZE
#define OSC_USB_STACK_SIZE 1536
#endif

static WORKING_AREA(waUsbThd, OSC_USB_STACK_SIZE);
static msg_t OscUsbSerialThread(void *arg)
{
  UNUSED(arg);
  bool val = 1;

  while (!usbserialIsActive())
    chThdSleepMilliseconds(50);

  while (!chThdShouldTerminate()) {
    int justGot = usbserialReadSlip(osc.usb.inBuf, sizeof(osc.usb.inBuf), 1000);
    if (justGot > 0) {
      ledSetValue(val);
      val = !val;
      chMtxLock(&osc.usb.lock);
      oscReceivePacket(USB, osc.usb.inBuf, justGot);
      oscSendPendingMessages(USB);
      chMtxUnlock();
    }
  }
  return 0;
}

static int oscSendMessageUSB(const char* data, int len)
{
  return usbserialWriteSlip(data, len, 1000);
}

bool oscUsbEnable(bool on)
{
  if (on && osc.usbThd == 0) {
    chMtxInit(&osc.usb.lock);
    osc.usb.sendMessage = oscSendMessageUSB;
    osc.usbThd = chThdCreateStatic(waUsbThd, sizeof(waUsbThd), NORMALPRIO, OscUsbSerialThread, NULL);
    return true;
  }
  if (!on && osc.usbThd != 0) {
    chThdTerminate(osc.usbThd);
    osc.usbThd = 0;
    return true;
  }
  return false;
}

#endif // MAKE_CTRL_USB

#ifdef MAKE_CTRL_NETWORK

#ifndef OSC_UDP_STACK_SIZE
#define OSC_UDP_STACK_SIZE 1536
#endif

static WORKING_AREA(waUdpThd, OSC_UDP_STACK_SIZE);
static msg_t OscUdpThread(void *arg) {
  UNUSED(arg);

  while ((osc.udpsock = udpOpen()) < 0)
    chThdSleepMilliseconds(500);

  udpBind(osc.udpsock, osc.udpListenPort);

  while (!chThdShouldTerminate()) {
    int justGot = udpRead(osc.udpsock, osc.udp.inBuf, sizeof(osc.udp.inBuf), &osc.udpReplyAddress, 0);
    if (justGot > 0) {
      chMtxLock(&osc.udp.lock);
      oscReceivePacket(UDP, osc.udp.inBuf, justGot);
      oscSendPendingMessages(UDP);
      chMtxUnlock();
    }
  }
  return 0;
}

static int oscSendMessageUDP(const char* data, int len)
{
  return udpWrite(osc.udpsock, data, len, osc.udpReplyAddress, osc.udpReplyPort);
}

bool oscUdpEnable(bool on)
{
  if (on && osc.udpThd == 0) {
    osc.udpListenPort = OSC_UDP_DEFAULT_PORT;
    oscUdpReplyPort();
    osc.udp.sendMessage = oscSendMessageUDP;
    chMtxInit(&osc.udp.lock);
    osc.udpThd = chThdCreateStatic(waUdpThd, sizeof(waUdpThd), NORMALPRIO, OscUdpThread, NULL);
    return true;
  }
  if (!on && osc.udpThd != 0) {
    chThdTerminate(osc.udpThd);
    osc.udpThd = 0;
    return true;
  }
  return false;
}

void oscUdpSetReplyPort(int port)
{
  if (osc.udpReplyPort != port) {
    osc.udpReplyPort = port;
    eepromWrite(EEPROM_OSC_UDP_SEND_PORT, port);
  }
}

int oscUdpReplyPort()
{
  if (osc.udpReplyPort == 0) { // uninitialized
    osc.udpReplyPort = eepromRead(EEPROM_OSC_UDP_SEND_PORT);
    if (osc.udpReplyPort < 0 || osc.udpReplyPort > 65536)
      osc.udpReplyPort = OSC_UDP_DEFAULT_PORT;
  }
  return osc.udpReplyPort;
}

void oscUdpSetListenPort(int port)
{
  if (osc.udpListenPort != port) {
    osc.udpListenPort = port;
    eepromWrite(EEPROM_OSC_UDP_LISTEN_PORT, port);
  }
}

int oscUdpListenPort()
{
  if (osc.udpListenPort == 0) { // uninitialized
    osc.udpListenPort = eepromRead(EEPROM_OSC_UDP_LISTEN_PORT);
    if (osc.udpListenPort < 0 || osc.udpListenPort > 65536)
      osc.udpListenPort = OSC_UDP_DEFAULT_PORT;
  }
  return osc.udpListenPort;
}

#endif // MAKE_CTRL_NETWORK

static WORKING_AREA(waAutosendThd, OSC_AUTOSEND_STACK_SIZE);
static msg_t OscAutosendThread(void *arg)
{
  UNUSED(arg);
  uint8_t i;
  const OscNode* node;
  OscChannelData* chd;

  while (!chThdShouldTerminate()) {
    if (osc.autosendDestination == NONE) {
      sleep(250);
    }
    else {
      chd = oscGetChannelByType(osc.autosendDestination);
      i = 0;
      node = oscRoot.children[i++];
      chMtxLock(&chd->lock);
      while (node != 0) {
        if (node->autosender != 0)
          node->autosender(osc.autosendDestination);
        node = oscRoot.children[i++];
      }
      oscSendPendingMessages(osc.autosendDestination);
      chMtxUnlock();
      sleep(osc.autosendPeriod);
    }
  }
  return 0;
}

void oscAutosendEnable(bool enabled)
{
  if (enabled && osc.autosendThd == 0) {
    // load up the interval and destination, and start the thread
    oscAutosendInterval();
    oscAutosendDestination();
    osc.autosendThd = chThdCreateStatic(waAutosendThd, sizeof(waAutosendThd), NORMALPRIO, OscAutosendThread, NULL);
  }
  else if (!enabled && osc.autosendThd != 0) {
    chThdTerminate(osc.autosendThd);
    osc.autosendThd = 0;
  }
}

OscChannel oscAutosendDestination()
{
  if (osc.autosendDestination == 0) { // uninitialized
    osc.autosendDestination = eepromRead(EEPROM_OSC_ASYNC_DEST);
    bool valid = false;
    #ifdef MAKE_CTRL_USB
    if (osc.autosendDestination == USB)
      valid = true;
    #endif
    #ifdef MAKE_CTRL_NETWORK
    if (osc.autosendDestination == UDP)
      valid = true;
    #endif
    if (!valid)
      osc.autosendDestination = NONE;
  }
  return osc.autosendDestination;
}

void oscSetAutosendDestination(OscChannel oc)
{
  if (osc.autosendDestination != oc) {
    osc.autosendDestination = oc;
    eepromWrite(EEPROM_OSC_ASYNC_DEST, oc);
  }
}

uint32_t oscAutosendInterval()
{
  if (osc.autosendPeriod == 0) { // uninitialized
    osc.autosendPeriod = eepromRead(EEPROM_OSC_ASYNC_INTERVAL);
    if (osc.autosendPeriod < 1 || osc.autosendPeriod > OSC_AUTOSEND_MAX_INTERVAL)
      osc.autosendPeriod = OSC_AUTOSEND_DEFAULT_INTERVAL;
  }
  return osc.autosendPeriod;
}

void oscSetAutosendInterval(uint32_t interval)
{
  if (interval != osc.autosendPeriod && interval > 1 && interval < OSC_AUTOSEND_MAX_INTERVAL) {
    osc.autosendPeriod = interval;
    eepromWrite(EEPROM_OSC_ASYNC_INTERVAL, interval);
  }
}

OscChannelData* oscGetChannelByType(OscChannel ct)
{
#ifdef MAKE_CTRL_USB
  if (ct == USB) return &osc.usb;
#endif
#ifdef MAKE_CTRL_NETWORK
  if (ct == UDP) return &osc.udp;
#endif
  return 0;
}

void oscLockChannel(OscChannel ct)
{
#ifdef MAKE_CTRL_USB
  if (ct == USB) chMtxLock(&osc.usb.lock); return;
#endif
#ifdef MAKE_CTRL_NETWORK
  if (ct == UDP) chMtxLock(&osc.udp.lock); return;
#endif
}

void oscUnlockChannel(OscChannel ct)
{
  UNUSED(ct);
  chMtxUnlock();
}

void oscResetChannel(OscChannelData* channel)
{
  channel->outBufRemaining = sizeof(channel->outBuf);
  channel->outBufPtr = channel->outBuf;
  channel->outMsgCount = 0;
}

/*
  A new packet has arrived.  Check if it's a single message or a
  bundle and process accordingly. If any response messages were
  generated during processing, send them off.
*/
void oscReceivePacket(OscChannel ch, char* data, uint32_t len)
{
  uint32_t length = len;
  if (data[0] == '/') { // single message
    oscReceiveMessage(ch, data, length);
  }
  else if (data[0] == '#') { // bundle
    data += 16; // skip timetag
    length -= 16;
    while (length > 0) {
      uint32_t msglen; // each message preceded by int32 length
      data = oscDecodeInt32(data, &length, (int*)&msglen);
      if (msglen > len) // we got a bogus length
        break;
      oscReceivePacket(ch, data, msglen);
      data += msglen;
      length -= msglen;
    }
  }
}

/*
  A new message has arrived.  Extract the data from it, then
  dispatch it to any nodes that match it.
*/
void oscReceiveMessage(OscChannel ch, char* data, uint32_t len)
{
  // if the last char is a /, treat it as a namespace query
  if (data[strlen(data) - 1] == '/') {
    oscNameSpaceQuery(ch, data + 1, data, &oscRoot);
    return;
  }

  // otherwise, treat as a normal message
  uint32_t length = oscPaddedStrlen(data);
  if (len < length)
    return;
  // number of data items is the length of the typetag
  uint32_t datalen = strlen(data + length) - 1; // don't take the leading , into account
  if (datalen > OSC_MAX_DATA_ITEMS) // make sure we don't blow the stack
    return;
  OscData d[datalen];
  if (datalen == oscExtractData(data + length, len, d, datalen)) {
    oscDispatchNode(ch, data + 1, data, &oscRoot, d, datalen);
  }
}

/*
 * Dispatch data to matching nodes.
 * Range nodes are treated specially - check if the direct child has
 * a matching handler & trigger it.  Otherwise, descend looking for a
 * child with a matching handler by name only.
 */
bool oscDispatchNode(OscChannel ch, char* addr, char* fulladdr, const OscNode* node, OscData data[], int datalen)
{
  char* nextPattern = strchr(addr, '/');
  if (nextPattern != 0)
    *nextPattern++ = 0;

  if (node->handler != NULL) {
    node->handler(ch, fulladdr, 0, data, datalen);
    return true;
  }

  uint8_t i;
  if (node->range > 0) {
    // as part of our cheat, ranges can only be the second to last node.
    // we jump down a level here since we are planning on getting to the handler
    // without traversing the tree any further
    for (i = 0; node->children[i] != 0; i++) {
      if (node->children[i]->handler && oscPatternMatch(nextPattern, node->children[i]->name)) {
        OscRange r;
        if (oscNumberMatch(addr, node->rangeOffset, node->range, &r)) {
          *(addr - 1) = 0;
          char *endofaddr = fulladdr + strlen(fulladdr);
          while (oscRangeHasNext(&r)) {
            int idx = oscRangeNext(&r);
            // recreate an address specific to this index, in the case that we got here
            // through a pattern match
            siprintf(endofaddr, "/%d/%s", idx, node->children[i]->name);
            node->children[i]->handler(ch, fulladdr, idx, data, datalen);
          }
          return true;
        }
      }
    }
    // replace the nulls we stuck in the address string
    *--addr = '/';
    *(nextPattern - 1) = '/';
  }
  // otherwise, go down to the next level and try some more
  for (i = 0; node->children[i] != 0; ++i) {
    if (oscPatternMatch(addr, node->children[i]->name)) {
      *(nextPattern - 1) = '/'; // replace this - we nulled it earlier
      if (oscDispatchNode(ch, nextPattern, fulladdr, node->children[i], data, datalen))
        return true;
    }
  }
  return false;
}

static void oscNameSpaceQueryEndpoint(OscChannel ch, char *fulladdr, const OscNode* node)
{
  uint8_t i;
  char *endoforiginal = fulladdr + strlen(fulladdr);
  for (i = 0; node->children[i] != 0; i++) {
    // we strcat into the original message buf here to save space - it's ok
    // since we've already got the data out of it that we need
    strcat(fulladdr, node->children[i]->name);
    oscCreateMessage(ch, fulladdr, 0, 0);
    // each time we strcat, we need to replace the null so subsequent children can be handled
    *endoforiginal = 0;
  }
}

static void oscNameSpaceQueryRangeEndpoint(OscChannel ch, char *fulladdr, const OscNode* node)
{
  OscRange r;
  char *endoforiginal = fulladdr + strlen(fulladdr);
  oscNumberMatch("*", node->rangeOffset, node->range, &r);
  while (oscRangeHasNext(&r)) {
    siprintf(endoforiginal, "/%d", oscRangeNext(&r));
    oscCreateMessage(ch, fulladdr, 0, 0);
  }
}

/*
  A msg with an address ending in / came in.  Treat it as a namespace query,
  and return any OscNodes under the requested namespace.
*/
bool oscNameSpaceQuery(OscChannel ch, char* addr, char *fulladdr, const OscNode* node)
{
  char* nextpattern = strchr(addr, '/');
  if (nextpattern != 0) {
    *nextpattern++ = 0;
  }
  // special case check for the root node, since it doesn't have a name
  else if (node == &oscRoot) {
    oscNameSpaceQueryEndpoint(ch, fulladdr, node);
    return true;
  }

  // do a simple strcmp - don't need to match patterns for this
  if (strcmp(addr, node->name) ==  0) {
    if (node->range > 0 && *nextpattern == 0)
      oscNameSpaceQueryRangeEndpoint(ch, fulladdr, node);
    else {
      *(nextpattern - 1) = '/'; // replace this - we nulled it earlier
      oscNameSpaceQueryEndpoint(ch, fulladdr, node);
    }
    return true;
  }

  // or try the next level down
  uint8_t i;
  for (i = 0; node->children[i] != 0; i++) {
    if (oscPatternMatch(addr, node->children[i]->name)) {
      *(nextpattern - 1) = '/'; // replace this - we nulled it earlier
      if (oscNameSpaceQuery(ch, addr, fulladdr, node->children[i]))
        return true;
    }
  }
  return false;
}

/*
  We expect to be pointing just past the address pattern.
  Find the type tag, then find the start of the data.
  Pull out values from the data segment according to what the
  typetag tells us we should find.
*/
uint32_t oscExtractData(char* buf, uint32_t len, OscData data[], int datacount)
{
  int items = 0;
  if (buf[0] != ',') // beginning of typetag should be ,
    return 0;
  
  char* typetag = buf + 1; // skip the ,
  uint32_t paddedlen = oscPaddedStrlen(buf); // skip to end of typetag
  if (len < paddedlen)
    return items;
  buf += paddedlen;
  len -= paddedlen;

  while (*typetag != 0 && buf != 0 && len > 0 && items < datacount) {
    switch (*typetag++) {
      case 'i': {
        int i;
        if ((buf = oscDecodeInt32(buf, &len, &i)) != NULL) {
          data[items].type = INT;
          data[items++].value.i = i;
        }
        break;
      }
      case 'f': {
        float f;
        if ((buf = oscDecodeFloat32(buf, &len, &f)) != NULL) {
          data[items].type = FLOAT;
          data[items++].value.f = f;
        }
        break;
      }
      case 's': {
        char* s;
        if ((buf = oscDecodeString(buf, &len, &s)) != NULL) {
          data[items].type = STRING;
          data[items++].value.s = s;
        }
        break;
      }
      case 'b': {
        char* b;
        uint32_t bloblen;
        if ((buf = oscDecodeBlob(buf, &len, &b, &bloblen)) != NULL) {
          data[items].type = BLOB;
          data[items++].value.b = b;
          // TODO - need some way to represent the blob len
        }
        break;
      }
    }
  }
  return items;
}

/*
 * Write out an OSC timetag
 */
static char* oscWriteTimetag(char* data, uint32_t* len, int a, int b)
{
  if (*len < 8)
    return 0;
  if ((data = oscEncodeInt32(data, len, a)) == NULL)
    return 0;
  return oscEncodeInt32(data, len, b);
}

static char* oscCreateBundle(char* data, uint32_t* len, int a, int b )
{
  if ((data = oscEncodeString(data, len, "#bundle")) == NULL)
    return 0;
  return oscWriteTimetag(data, len, a, b);
}

static char* oscDoCreateMessage(OscChannelData* chd, const char* address, OscData* data, int datacount)
{
  char* buf = chd->outBufPtr;
  uint32_t* len = &chd->outBufRemaining;
  /*
    if this is the first msg in the buffer, write bundle
    info in there in case the outgoing message ends up
    being a bundle - can always be skipped if not needed
  */
  if (chd->outMsgCount == 0) {
    if ((buf = oscCreateBundle(buf, len, 0, 0)) == NULL)
      return 0;
  }

  if (*len < sizeof(int))
    return 0;
  char* lenp = buf; // where to stick this message's length once we know it
  buf += sizeof(int);
  *len -= sizeof(int);
  char* messagestart = buf;

  // do the address
  if ((buf = oscEncodeString(buf, len, address)) == NULL)
    return 0;

  // build up the typetag
  uint8_t i;
  char typetag[OSC_MAX_DATA_ITEMS + 2] = ","; // 2 = 1 for comma, 1 for terminator
  char* t = typetag + 1;
  for (i = 0; i < datacount; i++)
    *t++ = data[i].type;
  *t = 0; // null terminate
  if ((buf = oscEncodeString(buf, len, typetag)) == NULL)
    return 0;

  // now pack the data
  for (i = 0; i < datacount && buf != NULL; i++) {
    switch (data[i].type) {
      case INT:
        buf = oscEncodeInt32(buf, len, data[i].value.i);
        break;
      case FLOAT:
        buf = oscEncodeFloat32(buf, len, data[i].value.f);
        break;
      case STRING:
        buf = oscEncodeString(buf, len, data[i].value.s);
        break;
      case BLOB:
        buf = oscEncodeBlob(buf, len, data[i].value.b, 100);
        break;
      default: break;
    }
  }

  // write the length of this message
  uint32_t dummylen = sizeof(int);
  if (oscEncodeInt32(lenp, &dummylen, (buf - messagestart)) == NULL)
    return 0;
  if (buf != NULL)
    chd->outMsgCount++;

  chd->outBufPtr = buf;
  return buf;
}

// Create an OSC message given a number of data items.
bool oscCreateMessage(OscChannel ch, const char* address, OscData* data, int datacount)
{
  OscChannelData* chd = oscGetChannelByType(ch);
  bool rv = true;
  // Try to create the message. If it fails, send any messages
  // in the buffer and try again.
  if (oscDoCreateMessage(chd, address, data, datacount) == NULL) {
    oscSendPendingMessages(ch);
    oscResetChannel(chd);
    if (oscDoCreateMessage(chd, address, data, datacount) == NULL)
      rv = false;
  }
  return rv;
}

/*
 * Send any pending messages out via a channel's sendMessage() routine.
 */
int oscSendPendingMessages(OscChannel ch)
{
  OscChannelData* chd = oscGetChannelByType(ch);
  if (chd->outMsgCount == 0)
    return 0;
  // set the buffer and length up
  char* data = chd->outBuf;
  int len = sizeof(chd->outBuf) - chd->outBufRemaining;
  // if we only have 1 message, skip past the bundle preamble
  // which has already been written to the buffer
  if (chd->outMsgCount == 1) {
    // skip 8 bytes of "#bundle" + 8 bytes of timetag + 4 bytes of size
    data += 20;
    len -= 20;
  }
  (*chd->sendMessage)(data, len);
  oscResetChannel(chd);
  return 1;
}

#endif // OSC
