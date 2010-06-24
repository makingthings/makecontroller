
#include "core.h"
#ifdef OSC
#include "osc_data.h"
#include "string.h"

#define OSC_BYTE_ALIGN 4

/*
 * Get 8 bits of a 32 bit value
 * w - 32 bit word, i - index
 */
#define byteOfWord(w,i) ((uint8_t*)&w)[i]

/**********************************************************

                            Encoding
                            
**********************************************************/

static char* oscNullPad(char* buf, uint32_t* remaining, int elementsize)
{
  uint32_t padding = elementsize % OSC_BYTE_ALIGN;
  if (*remaining < padding || buf == 0)
    return 0;
  while (padding--) {
    *buf++ = 0;
    (*remaining)--;
  }
  return buf;
}

char* oscEncodeString(char* buf, uint32_t* remaining, const char* str)
{
  uint32_t len = strlen(str) + 1; // account for null pad
  uint32_t pad = len % OSC_BYTE_ALIGN;
  if (pad != 0) pad = (OSC_BYTE_ALIGN - pad);
  if (*remaining < len + pad)
    return 0;

  strcpy(buf, str);
  *remaining -= len;
  buf += len;
  while (pad-- > 0) {
    (*remaining)--;
    *buf++ = 0;
  }
  return buf;
}

char* oscEncodeInt32(char* buf, uint32_t* remaining, int i)
{
  if (*remaining < sizeof(int) || buf == 0)
    return 0;
  // to big endian
  *buf++ = byteOfWord(i, 3);
  *buf++ = byteOfWord(i, 2);
  *buf++ = byteOfWord(i, 1);
  *buf++ = byteOfWord(i, 0);
  *remaining -= sizeof(int);
  return buf;
}

char* oscEncodeFloat32(char* buf, uint32_t* remaining, float f)
{
  if (*remaining < sizeof(float) || buf == 0)
    return 0;
  // to big endian
  *buf++ = byteOfWord(f, 3);
  *buf++ = byteOfWord(f, 2);
  *buf++ = byteOfWord(f, 1);
  *buf++ = byteOfWord(f, 0);
  *remaining -= sizeof(float);
  return buf;
}

char* oscEncodeBlob(char* buf, uint32_t* remaining, const char* b, uint32_t len)
{
  if (*remaining < len || buf == 0)
    return 0;
  buf = oscEncodeInt32(buf, remaining, len);
  *remaining -= (len + 4); // account for 4 bytes of len itself 
  while (len--)
    *buf++ = *b++;
  return oscNullPad(buf, remaining, len);
}

/**********************************************************

                            Decoding
                            
**********************************************************/

char* oscDecodeInt32(char* buf, uint32_t* remaining, int* value)
{
  if (*remaining < sizeof(int) || buf == 0)
    return 0;
  // to little endian
  byteOfWord(*value, 3) = *buf++;
  byteOfWord(*value, 2) = *buf++;
  byteOfWord(*value, 1) = *buf++;
  byteOfWord(*value, 0) = *buf++;
  *remaining -= sizeof(int);
  return buf;
}

char* oscDecodeFloat32(char* buf, uint32_t* remaining, float* value)
{
  if (*remaining < sizeof(float) || buf == 0)
    return 0;
  // to little endian
  byteOfWord(*value, 3) = *buf++;
  byteOfWord(*value, 2) = *buf++;
  byteOfWord(*value, 1) = *buf++;
  byteOfWord(*value, 0) = *buf++;
  *remaining -= sizeof(float);
  return buf;
}

/*
  Sets string to the string data in the buffer.
  Doesn't do any copying, so make sure the data hangs around.
*/
char* oscDecodeString(char* buf, uint32_t* remaining, char** str)
{
  if (buf == 0)
    return 0;
  *str = buf;
  uint32_t paddedlen = oscPaddedStrlen(buf);
  if (paddedlen > *remaining)
    return 0;

  buf += paddedlen;
  *remaining -= paddedlen;
  return buf;
}

char* oscDecodeBlob(char* buf, uint32_t* remaining, char** blob, uint32_t* len)
{
  if (buf == 0)
    return 0;
  buf = oscDecodeInt32(buf, remaining, (int*)len);
  if (*remaining < *len)
    return 0;
  *blob = buf;
  *remaining -= *len;
  buf += *len;
  return buf;
}

/*
 * Calculate the total length of an osc string,
 * including null padding
 */
int oscPaddedStrlen(const char* str) {
  int len = strlen(str) + 1; // account for null pad
  int pad = len % OSC_BYTE_ALIGN;
  if (pad != 0)
    len += (OSC_BYTE_ALIGN - pad);
  return len;
}

#endif // OSC
