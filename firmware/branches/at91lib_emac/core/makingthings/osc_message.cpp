

#include "osc_message.h"
#include <string.h>
#include <stdlib.h>

int OscMessage::addressElementAsInt( int element, bool* ok )
{
  if(ok)
    *ok = false;
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ ) // step to the beginning of the address element
    return false;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return false;
  }
  if(ok)
    *ok = true;
  return atoi(p);
}

float OscMessage::addressElementAsFloat( int element, bool* ok )
{
  if(ok)
    *ok = false;
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ ) // step to the beginning of the address element
    return 0;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return 0;
  }
  if(ok)
    *ok = true;
  return atof(p);
}

char* OscMessage::addressElementAsString( int element )
{
  if( !address )
    return 0;
  int j;
  const char* p = strchr(address, '/'); // should give us the very first char of the OSC message
  if( !p++ )
    return 0;
  for( j = 0; j < element; j++ )
  {
    p = strchr( p, '/');
    if(!p++)
      return 0;
  }
  return (char*)p;
}

int OscMessage::dataItemAsInt( int index, bool* ok )
{
  if(ok)
    *ok = false;
  if( index >= data_count || (data_items[index].type != oscInt) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].i;
}

float OscMessage::dataItemAsFloat( int index, bool* ok )
{
  if(ok)
    *ok = false;
  if( index >= data_count || (data_items[index].type != oscFloat) )
    return 0;
  if(ok)
    *ok = true;
  return data_items[index].f;
}

char* OscMessage::dataItemAsString( int index )
{
  if( index >= data_count || (data_items[index].type != oscString) )
    return 0;
  return data_items[index].s;
}

char* OscMessage::dataItemAsBlob( int index, int* blob_len )
{
  if( index >= data_count || (data_items[index].type != oscBlob) )
    return 0;
  char* p = data_items[index].s;
  *blob_len = *(int*)p;
  return p + 4;
}



