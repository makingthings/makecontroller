

#ifndef OSC_MESSAGE_H
#define OSC_MESSAGE_H

#define OSC_MSG_MAX_DATA_ITEMS 20

enum OscDataType { oscInt, oscFloat, oscString, oscBlob };

typedef struct OscData
{
  union
  {
    int i;
    float f;
    char* s;
    void* b;
  };
  OscDataType type;
};

class OscMessage
{
public:
  OscMessage( ) { data_count = 0; }
  char* address;
  OscData data_items[OSC_MSG_MAX_DATA_ITEMS];
  int data_count;
  
  int   addressElementAsInt( int element, bool* ok = 0 );
  float addressElementAsFloat( int element, bool* ok = 0 );
  char* addressElementAsString( int element );
  
  int   dataItemAsInt( int index, bool* ok = 0 );
  float dataItemAsFloat( int index, bool* ok = 0 );
  char* dataItemAsString( int index );
  char* dataItemAsBlob( int index, int* blob_len );
};

#endif // OSC_MESSAGE_H
