


#include "config.h"
#if 1 //def OSC

#include "osc.h"
#include "osc_patternmatch.h"
#include "osc_data.h"
#include "ch.h"
#include "core.h"
#include <string.h>

#ifndef OSC_IN_BUF_SIZE
#define OSC_IN_BUF_SIZE 512
#endif

#ifndef OSC_OUT_BUF_SIZE
#define OSC_OUT_BUF_SIZE 512
#endif

#define OSC_UDP_DEFAULT_PORT 10000

typedef int (*OscSendMsg)(const char* data, int len);

typedef struct OscChannelData_t {
  Mutex lock;
  int outMsgCount;
  int outBufRemaining;
  char* outBufPtr;
  char outBuf[OSC_OUT_BUF_SIZE];
  char inBuf[OSC_IN_BUF_SIZE];
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
#endif
} Osc;

static void oscReceivePacket(OscChannel ch, char* data, int len);
static void oscReceiveMessage(OscChannel ch, char* data, int len);
static void oscResetChannel(OscChannelData* ch);
static int  oscExtractData(char* buf, int len, OscData data[], int maxdata);
static void oscDispatchNode(OscChannel ch, char* addr, char* fulladdr,
                              const OscNode* node, OscData d[], int datalen);

static Osc osc;
extern const OscNode oscRootNode; // must be defined by the user

#ifdef MAKE_CTRL_USB

#ifndef OSC_USB_STACK_SIZE
#define OSC_USB_STACK_SIZE 256
#endif

static WORKING_AREA(waUsbThd, OSC_USB_STACK_SIZE);
static msg_t OscUsbSerialThread(void *arg) {
  UNUSED(arg);
  int justGot;

  while (!usbserialIsActive())
    chThdSleepMilliseconds(10);

  while(!chThdShouldTerminate()) {
    justGot = usbserialReadSlip(osc.usb.inBuf, OSC_IN_BUF_SIZE, 10000);
    if (justGot > 0) {
      chMtxLock(&osc.usb.lock);
      oscReceivePacket(USB, osc.usb.inBuf, justGot);
      chMtxUnlock();
    }
  }
  return 0;
}

static int oscSendMessageUSB(const char* data, int len)
{
  return usbserialWriteSlip(data, len, 100);
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
//    chThdWait(osc.usbThd);
    osc.usbThd = 0;
    return true;
  }
  return false;
}

#endif // MAKE_CTRL_USB

#ifdef MAKE_CTRL_NETWORK

#ifndef OSC_UDP_STACK_SIZE
#define OSC_UDP_STACK_SIZE 1024
#endif

static WORKING_AREA(waUdpThd, OSC_UDP_STACK_SIZE);
static msg_t OscUdpThread(void *arg) {
  UNUSED(arg);
  int justGot;

  chMtxInit(&osc.udp.lock);

  while ((osc.udpsock = udpOpen()) < 0)
    chThdSleepMilliseconds(500);

  udpBind(osc.udpsock, 10000);

  while(!chThdShouldTerminate()) {
    justGot = udpReadFrom(osc.udpsock, osc.udp.inBuf, OSC_IN_BUF_SIZE, &osc.udpReplyAddress, 0);
    if (justGot > 0) {
      chMtxLock(&osc.udp.lock);
      oscReceivePacket(UDP, osc.udp.inBuf, justGot);
      chMtxUnlock();
    }
  }
  return 0;
}

static int oscSendMessageUDP(const char* data, int len)
{
  return udpWrite(osc.udpsock, data, len, osc.udpReplyAddress, osc.udpReplyPort);
}

bool oscUdpEnable(bool on, int port)
{
  if (on && osc.udpThd == 0) {
    osc.udpReplyPort = (port == -1) ? OSC_UDP_DEFAULT_PORT : port;
    osc.udp.sendMessage = oscSendMessageUDP;
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

#endif // MAKE_CTRL_NETWORK

static OscChannelData* oscGetChannelByType(OscChannel ct) {
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
void oscReceivePacket(OscChannel ch, char* data, int len)
{
  if (data[0] == '/') { // single message
    oscReceiveMessage(ch, data, len);
  }
  else if (data[0] == '#') { // bundle
    data += 16; // skip timetag
    len -= 16;
    while (len > 0) {
      int msglen; // each message preceded by int32 length
      data = oscDecodeInt32(data, &len, &msglen);
      if (msglen <= len)
        oscReceivePacket(ch, data, msglen);
      data += msglen;
      len -= msglen;
    }
  }
  oscSendPendingMessages(ch);
}

/*
  A new message has arrived.  Extract the data from it, then
  dispatch it to any nodes that match it.
*/
void oscReceiveMessage(OscChannel ch, char* data, int len)
{
  int length = oscPaddedStrlen(data);
  if (len < length)
    return;
  // number of data items is the length of the typetag
  int datalen = strlen(data + length) - 1; // don't take the leading , into account
  OscData d[datalen];
  if (datalen == oscExtractData(data + length, len, d, datalen))
    oscDispatchNode(ch, data + 1, data, &oscRootNode, d, datalen);
}

/*
 * Dispatch data to matching nodes.
 * Range nodes are treated specially - check if the direct child has
 * a matching handler & trigger it.  Otherwise, descend looking for a
 * child with a matching handler by name only.
 */
void oscDispatchNode(OscChannel ch, char* addr, char* fulladdr, const OscNode* node, OscData data[], int datalen)
{
  short i;
  char* nextPattern = strchr(addr, '/');
  if (nextPattern != 0)
    *nextPattern++ = 0;

  for (i = 0; node->children[i] != 0; i++) {
    if (node->children[i]->handler != NULL) {
      node->children[i]->handler(ch, fulladdr, 0, data, datalen);
    }
    else if (nextPattern != NULL) {
      if (oscPatternMatch(addr, node->children[i]->name)) {
        *(nextPattern - 1) = '/'; // replace this - we nulled it earlier
        oscDispatchNode(ch, nextPattern, fulladdr, node->children[i], data, datalen);
      }
      else if (node->children[i]->range > 0) {
        OscRange r;
        // as part of our cheat, ranges can only be the second to last node.
        // we jump down a level here since we are planning on getting to the handler
        // without traversing the tree any further
        const OscNode *n = node->children[i];
        if (oscNumberMatch(addr, n->rangeOffset, n->range, &r)) {
          while (oscRangeHasNext(&r)) {
            int j, idx = oscRangeNext(&r);
            for (j = 0; n->children[j] != 0; j++) {
              if (oscPatternMatch(nextPattern, n->children[j]->name) && n->children[j]->handler != NULL)
                n->children[j]->handler(ch, fulladdr, idx, data, datalen);
            }
          }
        }
      }
    }
  }
}

/*
  We expect to be pointing just past the address pattern.
  Find the type tag, then find the start of the data.
  Pull out values from the data segment according to what the
  typetag tells us we should find.
*/
int oscExtractData(char* buf, int len, OscData data[], int datacount)
{
  int items = 0;
  if (buf[0] != ',') // beginning of typetag should be ,
    return 0;
  
  char* typetag = buf + 1; // skip the ,
  int paddedlen = oscPaddedStrlen(buf); // skip to end of typetag
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
        int bloblen;
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
  Take a / delimited address string as input, and split it into
  an array of pointers to each element. Return the number of elements.
*/
int oscSplitAddress(char* addr, char* elems[], int maxelem)
{
  int i = 0;
  char* e = " ";
  while (i < maxelem && e != NULL) {
    e = strchr(addr, '/');
    if (e != NULL && ++e != NULL)
      elems[i++] = e;
  }
  return i;
}

/*
 * Write out an OSC timetag
 */
static char* oscWriteTimetag(char* data, int* len, int a, int b)
{
  if (*len < 8)
    return 0;
  if ((data = oscEncodeInt32(data, len, a)) == NULL)
    return 0;
  if ((data = oscEncodeInt32(data, len, b)) == NULL)
    return 0;
  return data;
}

static char* oscCreateBundle(char* data, int* len, int a, int b )
{
  if ((data = oscEncodeString(data, len, "#bundle")) == NULL)
    return 0;
  if ((data = oscWriteTimetag(data, len, a, b)) == NULL)
    return 0;
  return data;
}

static char* oscDoCreateMessage(OscChannelData* chd, const char* address, OscData* data, int datacount)
{
  char* buf = chd->outBufPtr;
  int* len = &chd->outBufRemaining;
  /*
    if this is the first msg in the buffer, write bundle
    info in there in case the outgoing message ends up
    being a bundle - can always be skipped if not needed
  */
  if (chd->outMsgCount == 0) {
    if ((buf = oscCreateBundle(buf, len, 0, 0)) == NULL)
      return 0;
  }

  char* lenp = buf; // where to stick this message's length
  buf += 4;
  *len -= 4;
  char* messagestart = buf;

  // do the address
  if ((chd->outBufPtr = oscEncodeString(chd->outBufPtr, len, address)) == NULL)
    return 0;

  // do the type
  uint8_t i;
  char typetag[28] = ",";
  char* t = typetag + 1;
  for (i = 0; i < datacount; i++) {
    switch (data[i].type) {
      case INT:    *t++ = 'i'; break;
      case FLOAT:  *t++ = 'f'; break;
      case STRING: *t++ = 's'; break;
      case BLOB:   *t++ = 'b'; break;
      default: break;
    }
  }
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
  int dummylen = 4;
  if (oscEncodeInt32(lenp, &dummylen, (messagestart - buf)) == NULL)
    return 0;

  if (buf != NULL)
    chd->outMsgCount++;
  return buf;
}

/*
 * Create an OSC message given a number of schema data items.
 */
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
  int len = OSC_OUT_BUF_SIZE - chd->outBufRemaining;
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
