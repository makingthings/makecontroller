


#include "osc.h"
#include "osc_patternmatch.h"
#include "osc_data.h"
#include "schema.h"
#include <ch.h>
#include "usb_serial.h"
#include "core.h"
#include "string.h"

#ifndef OSC_SCHEMA_DATA_COUNT
#define OSC_SCHEMA_DATA_COUNT 8
#endif

#ifndef OSC_IN_BUF_SIZE
#define OSC_IN_BUF_SIZE 512
#endif

#ifndef OSC_OUT_BUF_SIZE
#define OSC_OUT_BUF_SIZE 512
#endif

#ifndef OSC_MAX_ADDR_ELEMENTS
#define OSC_MAX_ADDR_ELEMENTS 6
#endif

typedef enum OscChannelType_t {
  UDP,
  USB
} OscChannelType;

typedef int (*OscSendMsg)(const char* data, int len);

typedef struct OscChannel_t {
  OscChannelType which;
  Semaphore outMsgLock;
  int outMsgCount;
  int outBufRemaining;
  char* outBufPtr;
  char outData[OSC_OUT_BUF_SIZE];
  char inData[OSC_IN_BUF_SIZE];
  SchemaData schemaData[OSC_SCHEMA_DATA_COUNT];
  OscSendMsg sendMessage;
} OscChannel;

typedef struct Osc_t {
#ifdef MAKE_CTRL_USB
  Thread* usbThread;
#endif
#ifdef MAKE_CTRL_NETWORK
  Thread* udpThread;
#endif
} Osc;

// static funcs
static void oscReceivePacket(OscChannel* channel, char* data, int len);
static void oscReceiveMessage(OscChannel* channel, char* data, int len);
static void oscDispatchNode(OscChannelType channel, const char** fulladdr, SchemaOperation op,
                            const char** pattern, const SchemaNode* node,
                            SchemaData* data, int datacount);
static void oscResetSchemaData(SchemaData* data, int count);
static int  oscExtractData(char* buf, int len, SchemaData* data, int datacount);
static void oscResetChannel(OscChannel* channel);

// outgoing messages
static int oscSendPendingMessages( OscChannel* ch );

// static data
static Osc osc;

// network stuff
#ifdef MAKE_CTRL_NETWORK

#ifndef OSC_UDP_STACK_SIZE
#define OSC_UDP_STACK_SIZE 256
#endif

static int oscUdpReplyPort;
static int oscUdpReplyAddress;

static OscChannel oscUdpChannel;
static WORKING_AREA(waUdpThread, OSC_UDP_STACK_SIZE);
#endif // MAKE_CTRL_NETWORK

// usb stuff
#ifdef MAKE_CTRL_USB

#ifndef OSC_USB_STACK_SIZE
#define OSC_USB_STACK_SIZE 256
#endif

static OscChannel oscUsbChannel;
static WORKING_AREA(waUsbThread, OSC_USB_STACK_SIZE);
#endif // MAKE_CTRL_USB


#ifdef MAKE_CTRL_USB
/*
 * Loop forever reading USB data and feeding it to the
 * OSC dispatcher.
 */
static msg_t OscUsbSerialThread(void *arg) {
  UNUSED(arg);
  int justGot;

  while(!usbserialIsActive())
    chThdSleepMilliseconds(10);

  while(!chThdShouldTerminate()) {
    if ((justGot = usbserialReadSlip( oscUsbChannel.inData, OSC_IN_BUF_SIZE, 10000)) > 0) {
      oscReceivePacket(&oscUsbChannel, oscUsbChannel.inData, justGot);
      oscResetChannel(&oscUsbChannel);
    }
    chThdYield();
  }
  return 0;
}

static int oscSendMessageUSB( const char* data, int len)
{
  return usbserialWriteSlip( data, len, 100 );
}

bool oscSetUsbListener(bool on) {
  if(on && osc.usbThread == 0) {
    oscUsbChannel.sendMessage = oscSendMessageUSB;
    osc.usbThread = chThdCreateStatic(waUsbThread, sizeof(waUsbThread), NORMALPRIO, OscUsbSerialThread, NULL);
    return true;
  }
  if(!on && osc.usbThread != 0) {
    chThdTerminate(osc.usbThread);
//    chThdWait(osc.usbThread);
    return true;
  }
  return false;
}

#endif // MAKE_CTRL_USB

#ifdef MAKE_CTRL_NETWORK
/*
 * Loop forever reading USB data and feeding it to the
 * OSC dispatcher.
 */
static msg_t OscUdpThread(void *arg) {
  UNUSED(arg);
  int justGot;

  while(!usbserialIsActive())
    chThdSleepMilliseconds(10);

  while(!chThdShouldTerminate()) {
    if ((justGot = usbserialReadSlip( oscUdpChannel.inData, OSC_IN_BUF_SIZE, 10000)) > 0) {
      oscReceivePacket(&oscUdpChannel, oscUdpChannel.inData, justGot);
      oscResetChannel(&oscUdpChannel);
    }
    chThdYield();
  }
  return 0;
}

static int oscSendMessageUDP( const char* data, int len)
{
  return usbserialWriteSlip( data, len, 100 );
}

bool oscSetUdpListener(bool on) {
  if(on && osc.udpThread == 0) {
    oscUdpChannel.sendMessage = oscSendMessageUDP;
    osc.usbThread = chThdCreateStatic(waUdpThread, sizeof(waUdpThread), NORMALPRIO, OscUdpThread, NULL);
    return true;
  }
  if(!on && osc.udpThread != 0) {
    chThdTerminate(osc.udpThread);
    return true;
  }
  return false;
}

#endif // MAKE_CTRL_NETWORK

static OscChannel* oscGetChannelByType(OscChannelType ct) {
  switch(ct) {
#ifdef MAKE_CTRL_USB
    case USB: return &oscUsbChannel;
#endif
#ifdef MAKE_CTRL_NETWORK
    case UDP: return &oscUdpChannel;
#endif
    default:  return NULL;
  }
}

void oscResetChannel(OscChannel* channel)
{
  channel->outBufRemaining = sizeof(channel->outData);
  channel->outBufPtr = channel->outData;
  channel->outMsgCount = 0;
}

/*
  A new packet has arrived.  Check if it's a single message or a
  bundle and process accordingly. If any response messages were
  generated during processing, send them off.
*/
void oscReceivePacket(OscChannel* channel, char* data, int len)
{
  if (data[0] == '/') { // single message
    oscReceiveMessage(channel, data, len);
  }
  else if (data[0] == '#') { // bundle
    data += 16; // skip timetag
    len -= 16;
    while ( len > 0 ) {
      int msglen; // each message preceded by int32 length
      data = oscDecodeInt32(data, &len, &msglen);
      if ( msglen <= len )
        oscReceivePacket(channel, data, msglen);
      data += msglen;
      len -= msglen;
    }
  }
  if(channel->outMsgCount > 0)
    oscSendPendingMessages(channel);
}

/*
  A new message has arrived.  Extract the data from it, then
  dispatch it to any nodes that match it.
*/
void oscReceiveMessage(OscChannel* channel, char* data, int len) {
  char* p = data;
  int addrlen = oscPaddedStrlen(p);
  if(len < addrlen)
    return;
  p += addrlen;
  len -= addrlen;
  // extract data
  int count = oscExtractData(p, len, channel->schemaData, OSC_SCHEMA_DATA_COUNT);
  SchemaOperation op = (count > 0) ? SET : GET;

  // create an array of 0-terminated address elements
  const char* addrElements[OSC_MAX_ADDR_ELEMENTS];
  p = data + 1;
  int i = 0;
  addrElements[i++] = p;
  while((p = strchr(p, '/')) != NULL) {
    *p++ = 0;
    if (i < OSC_MAX_ADDR_ELEMENTS)
      addrElements[i++] = p;
    else {
      i = OSC_MAX_ADDR_ELEMENTS - 1; // so the array is properly terminated below
      break;
    }
  }
  addrElements[i] = 0;

  // dispatch to nodes
  for( i = 0; i < schemaRootNode.childCount; i++ ) {
    if (oscPatternMatch(addrElements[0], schemaRootNode.childNodes[i]->name))
      oscDispatchNode(channel->which, addrElements, op, addrElements, schemaRootNode.childNodes[i], channel->schemaData, count);
  }

  oscResetSchemaData(channel->schemaData, OSC_SCHEMA_DATA_COUNT);
}

void oscResetSchemaData(SchemaData* data, int count)
{
  while(--count >= 0)
    data[count].type = UNDEF;
}

/*
 * Dispatch data to matching nodes.
 * Index nodes are treated specially - check if the direct child has
 * a matching endpoint & trigger it.  Otherwise, descend looking for a
 * child with a matching endpoint by name only.
 */
void oscDispatchNode(OscChannelType channel, const char** fulladdr,
                      SchemaOperation op, const char** pattern, const SchemaNode* node,
                      SchemaData* data, int datacount)
{
  short i;
  const char** nextPattern = pattern + 1;
  
  // check if this is an index node
  if(node->indexCount > 0) {
    OscRange r;
    if( oscNumberMatch(*pattern, node->indexOffset, node->indexCount, &r) ) {
      while( oscRangeHasNext(&r) ) {
        int idx = oscRangeNext(&r);
        short j;
        for(j = 0; j < node->childCount; j++) {
          if(oscPatternMatch(*nextPattern, node->childNodes[j]->name) &&
             node->childNodes[j]->endpoint != NULL)
          {
            node->childNodes[j]->endpoint(fulladdr, idx, op, data, datacount);
          }
        }
      }
    }
//      oscDispatchNode(channel, fulladdr, op, nextPattern, node->childNodes[i], data, datacount);
  }
  // otherwise, if the callback is defined at this level, call it and be done
  // if not, dispatch to available child nodes
  else { 
    if(node->endpoint != NULL) {
      node->endpoint(fulladdr, i, op, data, datacount);
    }
    else if (nextPattern != NULL) {
      for(i = 0; i < node->childCount; i++) {
        if (oscPatternMatch(*nextPattern, node->childNodes[i]->name) ||
            node->childNodes[i]->indexCount > 0)
        {
          oscDispatchNode(channel, fulladdr, op, nextPattern, node->childNodes[i], data, datacount);
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
int oscExtractData(char* buf, int len, SchemaData* data, int datacount)
{
  int items = 0;
  // skip past null padding to beginning of type tag
  while(len > 0 && *buf == 0) {
    buf++;
    len--;
  }

  if(*buf != ',') // beginning of typetag should be ,
    return items;
  
  char* typetag = buf + 1; // skip the ,
  int paddedlen = oscPaddedStrlen(buf); // skip to end of typetag
  if(len < paddedlen)
    return items;
  buf += paddedlen;
  len -= paddedlen;

  while(*typetag != 0 && buf != 0 && len > 0 && items < datacount) {
    switch(*typetag++) {
      case 'i': {
        int i;
        if ((buf = oscDecodeInt32(buf, &len, &i)) != NULL) {
          data[items].type = INT;
          data[items].i = i;
          items++;
        }
        break;
      }
      case 'f': {
        float f;
        if ((buf = oscDecodeFloat32(buf, &len, &f)) != NULL) {
          data[items].type = FLOAT;
          data[items].f = f;
          items++;
        }
        break;
      }
      case 's': {
        char* s;
        if ((buf = oscDecodeString(buf, &len, &s)) != NULL) {
          data[items].type = STRING;
          data[items].s = s;
          items++;
        }
        break;
      }
      case 'b': {
        char* b;
        int bloblen;
        if ((buf = oscDecodeBlob(buf, &len, &b, &bloblen)) != NULL) {
          data[items].type = BLOB;
          data[items].b = b;
          items++;
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
static char* oscWriteTimetag( char* data, int* len, int a, int b) {
  if ( *len < 8 )
    return NULL;

  if((data = oscEncodeInt32(data, len, a)) == NULL)
    return 0;
  if((data = oscEncodeInt32(data, len, b)) == NULL)
    return 0;

  return data;
}

static char* oscCreateBundle( char* data, int* len, int a, int b ) {
  // do the bundle bit
  if( (data = oscEncodeString( data, len, "#bundle" )) == NULL)
    return 0;

  // do the timetag
  if( (data = oscWriteTimetag( data, len, a, b )) == NULL)
    return 0;

  return data;
}

static char* oscDoCreateMessage(OscChannel* c, const char* address, SchemaData* data, int datacount) {
  char* buf = c->outBufPtr;
  int* len = &c->outBufRemaining;
  /*
   * if this is the first msg in the buffer, write bundle
   * info in there in case the outgoing message ends up
   * being a bundle - can always be skipped if not needed
   */
  if(c->outMsgCount == 0) {
    if( (buf = oscCreateBundle(buf, len, 0, 0)) == NULL)
      return 0;
  }

  char* lenp = buf; // where to stick this message's length
  buf += 4;
  *len -= 4;
  char* messagestart = buf;

  // do the address
  if( (c->outBufPtr = oscEncodeString( c->outBufPtr, len, address )) == NULL )
    return 0;

  // do the type
  short i;
  char typetag[OSC_SCHEMA_DATA_COUNT] = ",";
  char* t = typetag + 1;
  for(i = 0; i < datacount; i++) {
    switch(data[i].type) {
      case INT:
        *t++ = 'i'; break;
      case FLOAT:
        *t++ = 'f'; break;
      case STRING:
        *t++ = 's'; break;
      case BLOB:
        *t++ = 'b'; break;
      default: break;
    }
  }
  if( (buf = oscEncodeString( buf, len, typetag )) == NULL )
    return 0;

  // now pack the data
  for(i = 0; i < datacount && buf != NULL; i++) {
    switch(data[i].type) {
      case INT:
        buf = oscEncodeInt32(buf, len, data[i].i);
        break;
      case FLOAT:
        buf = oscEncodeFloat32(buf, len, data[i].f);
        break;
      case STRING:
        buf = oscEncodeString(buf, len, data[i].s);
        break;
      case BLOB:
        buf = oscEncodeBlob(buf, len, data[i].b, 100);
        break;
      default: break;
    }
  }

  // write the length of this message
  int dummylen = 4;
  if( oscEncodeInt32(lenp, &dummylen, (messagestart - buf)) == NULL)
    return 0;

  if(buf != NULL)
    c->outMsgCount++;
  return buf;
}

/*
 * Create an OSC message given a number of schema data items.
 */
bool oscCreateMessage( OscChannelType ct, const char* address, SchemaData* data, int datacount ) {

  OscChannel* c = oscGetChannelByType(ct);
  bool rv = true;
  // make sure nobody else is writing to the channel
  chSemWaitTimeout(&c->outMsgLock, 1000);
  // Try to create the message. If it fails, send any messages
  // in the buffer and try again.
  if(oscDoCreateMessage(c, address, data, datacount) == NULL) {
    oscSendPendingMessages(c);
    oscResetChannel(c);
    if(oscDoCreateMessage(c, address, data, datacount) == NULL)
      rv = false;
  }
  chSemSignal(&c->outMsgLock);
  return rv;
}

/*
 * Send any pending messages out via a channel's sendMessage() routine.
 */
int oscSendPendingMessages( OscChannel* ch )
{
  if (ch->outMsgCount == 0  || chSemWait(&ch->outMsgLock) != RDY_OK)
    return 0;

  // set the buffer and length up
  char* data = ch->outData;
  int len = OSC_OUT_BUF_SIZE - ch->outBufRemaining;

  // if we only have 1 message, skip past the bundle preamble
  // which has already been written to the buffer
  if ( ch->outMsgCount == 1 ) {
    // skip 8 bytes of "#bundle" + 8 bytes of timetag + 4 bytes of size
    data += 20;
    len -= 20;
  }

  (*ch->sendMessage)( data, len );
  oscResetChannel(ch);
  chSemSignal(&ch->outMsgLock);
  return 1;
}


