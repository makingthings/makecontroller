/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef JSON__H
#define JSON__H

#define JSON_MAX_DEPTH 20

class JsonEncoder
{
public:
  JsonEncoder( );
  char* objectOpen(char *buf, int *remaining);
  char* objectKey(const char *key, char *buf, int *remaining);
  char* objectClose(char *buf, int *remaining);
  char* arrayOpen(char *buf, int *remaining);
  char* arrayClose(char *buf, int *remaining);
  char* string(const char *string, char *buf, int *remaining);
  char* integer(int value, char *buf, int *remaining);
  char* boolean(bool value, char *buf, int *remaining);
  // todo - char* floating(char *buf, bool value, int *remaining);
  
protected:
  enum EncodeStep // state object for encoding
  {
    JSON_START,
    JSON_OBJ_START,
    JSON_OBJ_KEY,
    JSON_OBJ_VALUE,
    JSON_ARRAY_START,
    JSON_IN_ARRAY
  };
  struct EncodeState
  {
    EncodeStep steps[JSON_MAX_DEPTH]; // An array to keep track of the state of each step in the encoder.
    int depth; // The current depth of the encoder (how many elements have been opened).
  };
  void appendedAtom( );
  EncodeState state;
};

class JsonDecoder
{
public:
  JsonDecoder(void* context = 0);
  bool go(char* text, int len);
  void setIntCallback(bool(*int_callback)(void *ctx, int val));
  void setFloatCallback(bool(*float_callback)(void *ctx, float val));
  void setBoolCallback(bool(*bool_callback)(void *ctx, bool val));
  void setStringCallback(bool(*string_callback)(void *ctx, char *string, int len));
  void setStartObjCallback(bool(*start_obj_callback)(void *ctx));
  void setObjKeyCallback(bool(*obj_key_callback)(void *ctx, char *key, int len));
  void setEndObjCallback(bool(*end_obj_callback)(void *ctx));
  void setStartArrayCallback(bool(*start_array_callback)(void *ctx));
  void setEndArrayCallback(bool(*end_array_callback)(void *ctx));
protected:
  enum DecodeStep // state object for decoding
  {
    JSON_DECODE_OBJECT_START,
    JSON_DECODE_IN_OBJECT,
    JSON_DECODE_IN_ARRAY
  };
  struct DecodeState {
    DecodeStep steps[JSON_MAX_DEPTH]; // An array to keep track of each step of the decoder.
    int depth;                        // The current depth of the decoder (how many elements have been opened).
    bool gotcomma;                    // Used internally by the decoder.
    void* context;                    // A pointer to the user context.
    char* p;                          // A pointer to the data.
    int len;                          // The current length.
  };
  enum DecodeToken
  {
    token_true,
    token_false,
    token_colon,
    token_comma,
    token_eof,
    token_error,
    token_left_brace,
    token_left_bracket,
    token_null,
    token_right_brace,
    token_right_bracket,
    token_number,
    token_maybe_negative,
    token_string,
    token_unknown
  };
  struct Callbacks
  {
    bool(*null_callback)(void*);
    bool(*bool_callback)(void*, bool);
    bool(*int_callback)(void*, int);
    bool(*float_callback)(void*, float);
    bool(*string_callback)(void*, char*, int);
    bool(*start_obj_callback)(void*);
    bool(*obj_key_callback)(void*, char*, int);
    bool(*end_obj_callback)(void*);
    bool(*start_array_callback)(void*);
    bool(*end_array_callback)(void*);
  };
  DecodeToken getToken(char* text, int len);
  DecodeState state;
  Callbacks callbacks;
  
};

// Encode
// void JsonEncode_Init(JsonEncode_State* state);

#endif // JSON__H


