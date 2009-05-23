/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef JSON_H
#define JSON_H

#define JSON_MAX_DEPTH 20

/**
  Encode JSON strings.
  For decoding JSON, see JsonDecoder.
  
  From http://www.json.org: "JSON (JavaScript Object Notation) 
  is a lightweight data-interchange format. It is easy for humans to read and write. It is easy for 
  machines to parse and generate."

  JSON is quite widely used when communicating with web servers, or other network enabled devices.
  It's nice and small, and easy to work with.  It's quite well supported in many programming
  environments, so it's not a bad option for a communication format when you need to talk to other
  devices from the Make Controller.  

  \section Generating
  To generate JSON, create a JsonEncoder and use its API to add elements to the JSON string.  You need to provide
  a buffer to store the data in - specify this with a call to reset(), which should be called before each
  round of JSON generation.

  Each call to the encoder will return a pointer into the buffer, corresponding to the point
  right after the most recently added data.  If this is null, there was a problem (probably ran out of
  room in the buffer) and the JSON string is probably incomplete.

  \code
  #define MAX_JSON_LEN 256
  char jsonbuf[MAX_JSON_LEN];
  JsonEncoder encoder;
  encoder.reset(jsonbuf, MAX_JSON_LEN);

  encoder.objectOpen( );
  encoder.string("hello");
  encoder.integer(234);
  if(!encoder.objectClose())
  {
    // problem - we probably ran out of room in the buffer
  }
  int json_length = encoder.length(); // how long is the string we made?
  // otherwise, the string in jsonbuf looks like {"hello":234} - beautifully formatted JSON
  \endcode

  \b Note - the library will add the appropriate separators (: or , usually) to the string, 
  depending on the context of the objects and arrays you've opened, or other data you've inserted.

  \b Disclaimer - in an attempt to keep it as small and as simple as possible, this library is not 
  completely full featured.  It doesn't process escaped strings for you, and doesn't deal
  with some of the more exotic numeric representations outlined in the JSON specification.  
  It does, however, work quite well for most other JSON tasks.

  Thanks to YAJL (http://code.google.com/p/yajl-c) for some design inspiration.
  \par
*/
class JsonEncoder
{
public:
  JsonEncoder( );
  void reset(char* buffer, int size);
  char* objectOpen( );
  char* objectKey( const char *key );
  char* objectClose( );
  char* arrayOpen( );
  char* arrayClose( );
  char* string( const char *string );
  char* integer( int value );
  char* boolean( bool value );
  char* null( );
  // todo - char* floating(char *buf, bool value, int *remaining);
  char* encode( const char* fmt,  ... );
  int length();
  
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
  char* buf;
  int remaining;
  int original_buf_size;
};


class JsonHandler;

/**
  Decode JSON strings.
  To generate JSON, see JsonEncoder.

  JSON parsing is done using an event-based mechanism.  This means you can register for any parse events you care
  to hear about, and then be called back with their value as they're encountered in the JSON string.  There are 2 
  options for getting callbacks - either register individual functions, or use an object that inherits from JsonHandler.
  In each case, return true to continue parsing, or return false and parsing will stop.  
  
  \section inherit JsonHandler
  If you already have an object that should handle incoming JSON, it might be best to have it inherit from JsonHandler.
  Then, reimplement the methods on JsonHandler that you're interested in and you're all set.  Pass your class into
  the go() method and your methods will be called back.

  \code
  class MyHandler : public JsonHandler // descend from JsonHandler
  {
    JsonDecoder decoder; // decoder object
    bool onJsonInt(int val);
    bool onJsonObjStart();
    bool onJsonObjEnd();
  };
  
  bool MyHandler::onJsonInt(int val)
  {
    // do whatever you need to when you get an int from the decoder...
  }
  
  // this method might be called when we've gotten new JSON from somewhere...
  // maybe the web, maybe via USB...
  void MyHandler::onNewJsonData(char* data, int length)
  {
    // kick off the decoder, and pass ourselves in as the handler to be called back
    // now onJsonInt() will be called for each int in the JSON data
    decoder.go(data, length, this);
  }
  \endcode

  \section individual Individual Functions
  If you just want individual functions to be called back, rather than an object, you can register them
  directly with the JsonDecoder.  Then when you call go() you don't need to pass in a handler, and your functions
  will be called back appropriately.

  If you need to pass around some context that you would like available in each of the callbacks, 
  you can pass it to the JsonDecoder constructor and it will be passed to each of the callbacks you've registered.
  Otherwise, just leave it out if you don't need it.

  \code
  // first, define the functions that we want to be called back on
  bool on_obj_opened(void* ctx)
  {
    // will be called when an object has been opened...
    return true; // keep parsing
  }
  
  bool on_int(void *ctx, int val)
  {
    iny my_json_int = val;
    // called when an int is encountered...
    return true; // keep parsing
  }
  
  bool on_string(void *ctx, char *string, int len)
  {
    // called when a string is encountered...
    return true; // keep parsing
  }

  // Now, register these callbacks with the JSON decoder.
  JsonDecoder decoder; // create our decoder object
  decoder.setStartObjCallback(on_obj_opened);
  decoder.setIntCallback(on_int);
  decoder.setStringCallback(on_string);

  // Finally, run the decoder
  char jsonstr[] = "[{\"label\":\"value\",\"label2\":{\"nested\":234}}]";
  decoder.go(jsonstr, strlen(jsonstr));
  // now each of our callbacks will be triggered at the appropriate time
  \endcode

  Thanks to YAJL (http://code.google.com/p/yajl-c) for some design inspiration.
  \par
*/
class JsonDecoder
{
public:
  JsonDecoder(void* context = 0);
  void reset(void* context = 0);
  void setContext(void* context);
  bool go(char* text, int len, JsonHandler* handler = 0);
  void setIntCallback(bool(*int_callback)(void *ctx, int val));
  void setFloatCallback(bool(*float_callback)(void *ctx, float val));
  void setBoolCallback(bool(*bool_callback)(void *ctx, bool val));
  void setStringCallback(bool(*string_callback)(void *ctx, char *string, int len));
  void setStartObjCallback(bool(*start_obj_callback)(void *ctx));
  void setObjKeyCallback(bool(*obj_key_callback)(void *ctx, char *key, int len));
  void setEndObjCallback(bool(*end_obj_callback)(void *ctx));
  void setStartArrayCallback(bool(*start_array_callback)(void *ctx));
  void setEndArrayCallback(bool(*end_array_callback)(void *ctx));
  void setNullCallback(bool(*null_callback)(void *ctx));
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
  DecodeToken getToken(char* text);
  DecodeState state;
  Callbacks callbacks;
};

/**
  Interface for handling JSON parsing.
  Inherit from this class and reimplement any of the methods you want to be called by the 
  JsonDecoder.  Return false from any method to stop parsing.

  For convenience, copy and paste any of the following methods into your class definition:
  \code
  bool onJsonInt(void* ctx, int val);
  bool onJsonFloat(void* ctx, float val);
  bool onJsonBool(void* ctx, bool val);
  bool onJsonString(void* ctx, char *string, int len);
  bool onJsonStartObj(void* ctx);
  bool onJsonObjKey(void* ctx, char *key, int len);
  bool onJsonEndObj(void* ctx);
  bool onJsonStartArray(void* ctx);
  bool onJsonEndArray(void* ctx);
  bool onJsonNull(void* ctx);
  \endcode
*/
class JsonHandler
{
public:
  JsonHandler() {}
  virtual ~JsonHandler() {}
protected:
  /**
    An int has been decoded.
    @param val The decoded integer.
  */
  virtual bool onJsonInt(void* ctx, int val) {
    (void)ctx;
    (void)val;
    return true;
  }
  /**
    A float has been decoded.
    @param val The decoded float.
  */
  virtual bool onJsonFloat(void* ctx, float val) {
    (void)ctx;
    (void)val;
    return true;
  }
  /**
    A bool has been decoded.
    @param val The decoded bool.
  */
  virtual bool onJsonBool(void* ctx, bool val) {
    (void)ctx;
    (void)val;
    return true;
  }
  /**
    A string has been decoded.
    @param string A pointer to the string.
    @param len The length of the string.
  */
  virtual bool onJsonString(void* ctx, char *string, int len) {
    (void)ctx;
    (void)string;
    (void)len;
    return true;
  }
  /**
    A left bracket '{' has been decoded.
  */
  virtual bool onJsonStartObj(void* ctx) {
    (void)ctx;
    return true;
  }
  /**
    The key of an object has been decoded - in JSON, all keys are strings.
    @param key A pointer to the key string.
    @param len The length of the key string.
  */
  virtual bool onJsonObjKey(void* ctx, char *key, int len) {
    (void)ctx;
    (void)key;
    (void)len;
    return true;
  }
  /**
    A right bracket '}' has been decoded.
  */
  virtual bool onJsonEndObj(void* ctx) {
    (void)ctx;
    return true;
  }
  /**
    A left brace '[' has been decoded.
  */
  virtual bool onJsonStartArray(void* ctx) {
    (void)ctx;
    return true;
  }
  /**
    A right brace ']' has been decoded.
  */
  virtual bool onJsonEndArray(void* ctx) {
    (void)ctx;
    return true;
  }
  /**
    A null has been decoded.
  */
  virtual bool onJsonNull(void* ctx) {
    (void)ctx;
    return true;
  }

  friend class JsonDecoder;
};

#endif // JSON_H


