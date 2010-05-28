
#include "config.h"
#ifdef OSC
#include "osc_data.h"
#include "string.h"

/*
 * Get 8 bits of a 32 bit value
 * w - 32 bit word, i - index
 */
#define byteOfWord(w,i) ((unsigned char*)&w)[i]

static char* oscNullPad(char* buf, int* remaining);

/**********************************************************

                            Encoding
                            
**********************************************************/

char* oscNullPad(char* buf, int* remaining)
{
  if (*remaining <= 0 || buf == 0)
    return 0;
  int padding = *remaining % 4;
  while (padding--) {
    *buf++ = 0;
    (*remaining)--;
  }
  return buf;
}

char* oscEncodeString(char* buf, int* remaining, const char* str)
{
  while ((*remaining >= 0) && (*buf++ = *str++))
    (*remaining)--;
  return oscNullPad(buf, remaining);
}

char* oscEncodeInt32(char* buf, int* remaining, int i)
{
  if (*remaining < 4 || buf == 0)
    return 0;
  *buf++ = byteOfWord(i, 0);
  *buf++ = byteOfWord(i, 1);
  *buf++ = byteOfWord(i, 2);
  *buf++ = byteOfWord(i, 3);
  *remaining -= 4;
  return buf;
}

char* oscEncodeFloat32(char* buf, int* remaining, float f)
{
  if (*remaining < 4 || buf == 0)
    return 0;
  *buf++ = byteOfWord(f, 0);
  *buf++ = byteOfWord(f, 1);
  *buf++ = byteOfWord(f, 2);
  *buf++ = byteOfWord(f, 3);
  *remaining -= 4;
  return buf;
}

char* oscEncodeBlob(char* buf, int* remaining, const char* b, int len)
{
  if (*remaining < len || buf == 0)
    return 0;
  buf = oscEncodeInt32(buf, remaining, len);
  *remaining -= (len + 4); // account for 4 bytes of len itself 
  while (len--)
    *buf++ = *b++;
  return oscNullPad(buf, remaining);
}

/**********************************************************

                            Decoding
                            
**********************************************************/

char* oscDecodeInt32(char* buf, int* remaining, int* value)
{
  if (*remaining < 4 || buf == 0)
    return 0;
  // to little endian
  byteOfWord(*value, 3) = *buf++;
  byteOfWord(*value, 2) = *buf++;
  byteOfWord(*value, 1) = *buf++;
  byteOfWord(*value, 0) = *buf++;
  *remaining -= 4;
  return buf;
}

char* oscDecodeFloat32(char* buf, int* remaining, float* value)
{
  if (*remaining < 4 || buf == 0)
    return 0;
  // to little endian
  byteOfWord(*value, 3) = *buf++;
  byteOfWord(*value, 2) = *buf++;
  byteOfWord(*value, 1) = *buf++;
  byteOfWord(*value, 0) = *buf++;
  *remaining -= 4;
  return buf;
}

/*
  Sets string to the string data in the buffer.
  Doesn't do any copying, so make sure the data hangs around.
*/
char* oscDecodeString(char* buf, int* remaining, char** str)
{
  if (buf == 0)
    return 0;
  *str = buf;
  int paddedlen = oscPaddedStrlen(buf);
  if (paddedlen > *remaining)
    return 0;

  buf += paddedlen;
  *remaining -= paddedlen;
  return buf;
}

char* oscDecodeBlob(char* buf, int* remaining, char** blob, int* len)
{
  if(buf == 0)
    return 0;
  buf = oscDecodeInt32(buf, remaining, len);
  *blob = buf;
  *remaining -= *len;
  if (*remaining < 0)
    buf = 0;
  else
    buf += *len;
  
  return buf;
}

/*
 * Calculate the total length of an osc string,
 * including null padding
 */
int oscPaddedStrlen(const char* str) {
  int len = strlen(str) + 1; // account for null pad
  int pad = len % 4;
  if (pad != 0)
    len += ( 4 - pad );
  return len;
}

#endif // OSC
