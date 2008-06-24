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

#include "json.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>
#include <ctype.h>

#define JSON_MAX_DEPTH 32

static void JsonEncode_AppendedAtom(void);

typedef enum {
    JSON_START,
    JSON_OBJ_START,
    JSON_OBJ_KEY,
    JSON_OBJ_VALUE,
    JSON_ARRAY_START,
    JSON_IN_ARRAY
} JsonEncode_State;

struct Json_t
{
  JsonEncode_State states[JSON_MAX_DEPTH];
  int encode_depth;
} Json;

/** \defgroup JSON JSON
	The Make Controller JSON library provides a very small and very fast library for parsing and 
  generating json. 
  
  From http://www.json.org: "JSON (JavaScript Object Notation) 
  is a lightweight data-interchange format. It is easy for humans to read and write. It is easy for 
  machines to parse and generate."

  JSON is pretty widely used when communicating with web servers, or other network enabled devices.
  It's nice and small, and easy to work with.  It's quite well supported in many programming
  environments, so it's not a bad option for a communication format when you need to talk to other
  devices from the Make Controller.  
  
  \b Disclaimer - in an attempt to keep it as small and as simple as possible, this library is not 
  completely full featured at the moment.  It doesn't deal with escaped strings or some of the 
  more exotic numeric representations outlined in the JSON specification.  It does, however, work
  quite well for most other JSON tasks.

  \par Generating
  Generating JSON is pretty simple - just make successive calls to the API to add the desired
  elements to your string.  You're responsible for providing a buffer to store the string in, and
  a count of how many bytes are left in that buffer.  The API will update that count, so it's not
  too much trouble.

  \code
  #define MAX_JSON_LEN 256
  char jsonbuf[MAX_JSON_LEN];
  int remaining = MAX_JSON_LEN;

  char *p = jsonbuf; // keep a pointer to the current location
  p = JsonEncode_ObjectOpen(p, &remaining);
  p = JsonEncode_String(p, "hello", &remaining);
  p = JsonEncode_Int(p, 234, &remaining);
  p = JsonEncode_ObjectClose(p, &remaining);
  // now the string in jsonbuf looks like {"hello":234}, beautifully formatted JSON
  int json_len = MAX_JSON_LEN - remaining; // this is the total length of the string in jsonbuf
  \endcode

  Note that the library will add the appropriate separators (: or , usually) to the string, 
  depending on the context of the objects and arrays you've opened, or other data you've inserted.

  \par Parsing
  Parsing is done using an event-based mechanism.  This means you can register for any events you care
  to hear about, and then be called back with their value as they're encountered in the JSON string.
  Parsing does not support incremental parsing at the moment - you need to pass in the whole JSON
  string you want to parse.  The parser will get through as much of the string as it can.

  In each callback, return true to continue parsing, or return false and parsing will stop.  

  If you need to pass around some context that you would like available in each of the callbacks, 
  you can pass it to JsonDecode() and it will be passed to each of the callbacks you've registered.
  Otherwise, just pass 0 if you don't need it.

  \code
  // first, define the functions that we want to be called back on
  bool on_obj_opened(void* ctx)
  {
    // deal with an object being opened...
    return true; // keep parsing
  }
  
  bool on_int(void *ctx, int val)
  {
    iny my_json_int = val;
    // deal with a new int...
    return true; // keep parsing
  }
  
  bool on_string(void *ctx, char *string, int len)
  {
    // deal with a new string...
    return true; // keep parsing
  }

  // Now, register these callbacks with the JSON parser.
  JsonDecode_SetStartObjCallback(on_obj_opened);
  JsonDecode_SetIntCallback(on_int);
  JsonDecode_SetStringCallback(on_string);

  // Finally, run the parser.
  char jsonstr[] = "[{\"label\":\"value\",\"label2\":{\"nested\":234}}]";
  JsonDecode(jsonstr, strlen(jsonstr), 0);
  \endcode

  Thanks to YAJL (http://code.google.com/p/yajl-c) for some design inspiration.
  \par

	\ingroup Libraries
	@{
*/

/**
  Reset the internal state of the JSON system.
  Generally do this once you've successfully generated a string,
  and want to start on the next one.
*/
void JsonEncode_Reset( )
{
  Json.encode_depth = 0;
  Json.states[0] = JSON_START;
}

/**
  Open up a new JSON object.
  This adds an opening '{' to the json string.
  @param buf A pointer to the buffer holding the JSON string.
  @param remaining A pointer to the count of how many bytes are left in your JSON buffer.
  @return A pointer to the location in the JSON buffer after this element has been added, or NULL if there was no room.
*/
char* JsonEncode_ObjectOpen(char *buf, int *remaining)
{
  int len = 1;
  switch(Json.states[Json.encode_depth])
  {
    case JSON_ARRAY_START:
    case JSON_OBJ_START:
    case JSON_START:
    {
      if(*remaining < len)
        return NULL;
      memcpy(buf, "{", len);
      break;
    }
    case JSON_OBJ_KEY:
    case JSON_IN_ARRAY:
    {
      len += 1; // for ,
      if(*remaining < len)
        return NULL;
      memcpy(buf, ",{", len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      len += 1; // for :
      if(*remaining < len)
        return NULL;
      memcpy(buf, ":{", len);
      break;
    }
  }
  
  if(++Json.encode_depth > JSON_MAX_DEPTH)
    return NULL;
  Json.states[Json.encode_depth] = JSON_OBJ_START;
  (*remaining) -= len;
  return buf + len;
}

/**
  Set the key for a JSON object.
  This is a convenience function that simply calls JsonEncode_String().
  It is provided to help enforce the idea that the first member of a JSON
  object pair must be a string.
  @see JsonEncode_String()
*/
char* JsonEncode_ObjectKey(char *buf, const char *key, int *remaining)
{
  return JsonEncode_String(buf, key, remaining);
}

/**
  Close a JSON object.
  Adds a closing '}' to the string.
  @param buf A pointer to the buffer which contains your JSON string.
  @param remaining A pointer to an integer keeping track of how many bytes are left in your JSON buffer.
  @return A pointer to the location in the JSON buffer after this element has been added, or NULL if there was no room.
*/
char* JsonEncode_ObjectClose(char *buf, int *remaining)
{
  if(*remaining < 1)
    return NULL;
  memcpy(buf, "}", 1);
  Json.encode_depth--;
  JsonEncode_AppendedAtom();
  (*remaining)--;
  return buf + 1;
}

/**
  Open up a new JSON array.
  This adds an opening '[' to the json string.
  @param buf A pointer to the buffer holding the JSON string.
  @param remaining A pointer to the count of how many bytes are left in your JSON buffer.
  @return A pointer to the location in the JSON buffer after this element has been added, or NULL if there was no room.
*/
char* JsonEncode_ArrayOpen(char *buf, int *remaining)
{
  int len = 1;
  switch(Json.states[Json.encode_depth])
  {
    case JSON_ARRAY_START:
    case JSON_OBJ_START:
    case JSON_START:
    {
      if(*remaining < len)
        return NULL;
      memcpy(buf, "[", len);
      break;
    }
    case JSON_OBJ_KEY:
    case JSON_IN_ARRAY:
    {
      len += 1; // for ,
      if(*remaining < len)
        return NULL;
      memcpy(buf, ",[", len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      len += 1; // for :
      if(*remaining < len)
        return NULL;
      memcpy(buf, ":[", len);
      break;
    }
  }
  if(++Json.encode_depth > JSON_MAX_DEPTH)
    return NULL;
  Json.states[Json.encode_depth] = JSON_ARRAY_START;
  (*remaining) -= len;
  return buf + len;
}

/**
  Close an array.
  Adds a closing ']' to the string.
  @param buf A pointer to the buffer which contains your JSON string.
  @param remaining A pointer to the count of how many bytes are left in your JSON buffer.
  @return A pointer to the JSON buffer after this element has been added, or NULL if there was no room.
*/
char* JsonEncode_ArrayClose(char *buf, int *remaining)
{
  if(*remaining < 1)
    return NULL;
  memcpy(buf, "]", 1);
  Json.encode_depth--;
  JsonEncode_AppendedAtom();
  (*remaining)--;
  return buf + 1;
}

/**
  Add a string to the current JSON string.
  Depending on whether you've opened objects, arrays, or other inserted 
  other data, the approprate separating symbols will be added to the string.

  Doesn't do escaping or anything fancy like that, at the moment.

  @param buf A pointer to the buffer containing the JSON string.
  @param string The string to be added.
  @param remaining A pointer to the count of bytes remaining in the JSON buffer.
*/
char* JsonEncode_String(char *buf, const char *string, int *remaining)
{
  int string_len = strlen(string) + 2 /* quotes */;

  switch(Json.states[Json.encode_depth])
  {
    case JSON_ARRAY_START:
    case JSON_OBJ_START:
    {
      if(*remaining < string_len)
        return NULL;
      char temp[string_len+1];
      snprintf(temp, string_len+1, "\"%s\"", string);
      memcpy(buf, temp, string_len);
      break;
    }
    case JSON_OBJ_KEY:
    case JSON_IN_ARRAY:
    {
      string_len += 1; // for ,
      if(*remaining < string_len)
        return NULL;
      char temp[string_len+1];
      snprintf(temp, string_len+1, ",\"%s\"", string);
      memcpy(buf, temp, string_len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      string_len += 1; // for :
      if(*remaining < string_len)
        return NULL;
      char temp[string_len+1];
      snprintf(temp, string_len+1, ":\"%s\"", string);
      memcpy(buf, temp, string_len);
      break;
    }
    default:
      return NULL;
  }
  JsonEncode_AppendedAtom();
  (*remaining) -= string_len;
  return buf + string_len;
}

/**
  Add an int to a JSON string.

  @param buf A pointer to the buffer containing the JSON string.
  @param value The integer to be added.
  @param remaining A pointer to the count of bytes remaining in the JSON buffer.
*/
char* JsonEncode_Int(char *buf, int value, int *remaining)
{
  int int_len = 4;
  switch(Json.states[Json.encode_depth])
  {
    case JSON_ARRAY_START:
    {
      if(*remaining < int_len)
        return NULL;
      char temp[int_len+1];
      snprintf(temp, int_len+1, "%d", value);
      memcpy(buf, temp, int_len);
      int_len = strlen(temp);
      break;
    }
    case JSON_IN_ARRAY:
    {
      int_len += 1; // for ,
      if(*remaining < int_len)
        return NULL;
      char temp[int_len+1];
      snprintf(temp, int_len+1, ",%d", value);
      memcpy(buf, temp, int_len);
      int_len = strlen(temp);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      int_len += 1; // for :
      if(*remaining < int_len)
        return NULL;
      char temp[int_len+1];
      snprintf(temp, int_len+1, ":%d", value);
      int_len = strlen(temp);
      memcpy(buf, temp, int_len);
      break;
    }
    default:
      return NULL; // bogus state
  }
  JsonEncode_AppendedAtom();
  (*remaining) -= int_len;
  return buf + int_len;
}

/**
  Add a boolean value to a JSON string.

  @param buf A pointer to the buffer containing the JSON string.
  @param value The boolean value to be added.
  @param remaining A pointer to the count of bytes remaining in the JSON buffer.
*/
char* JsonEncode_Bool(char *buf, bool value, int *remaining)
{
  const char* boolval = (value) ? "true" : "false";
  int bool_len = strlen(boolval);
  switch(Json.states[Json.encode_depth])
  {
    case JSON_ARRAY_START:
    {
      if(*remaining < bool_len)
        return NULL;
      char temp[bool_len+1];
      snprintf(temp, bool_len+1, "%s", boolval);
      memcpy(buf, temp, bool_len);
      break;
    }
    case JSON_IN_ARRAY:
    {
      bool_len += 1; // for ,
      if(*remaining < bool_len)
        return NULL;
      char temp[bool_len+1];
      snprintf(temp, bool_len+1, ",%s", boolval);
      memcpy(buf, temp, bool_len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      bool_len += 1; // for :
      if(*remaining < bool_len)
        return NULL;
      char temp[bool_len+1];
      snprintf(temp, bool_len+1, ":%s", boolval);
      memcpy(buf, temp, bool_len);
      break;
    }
    default:
      return NULL; // bogus state
  }
  JsonEncode_AppendedAtom();
  (*remaining) -= bool_len;
  return buf + bool_len;
}

/*
  Called after adding a new value (atom) to a string in order
  to update the state appropriately.
*/
// static
void JsonEncode_AppendedAtom()
{
  switch(Json.states[Json.encode_depth])
  {
    case JSON_OBJ_START:
    case JSON_OBJ_KEY:
      Json.states[Json.encode_depth] = JSON_OBJ_VALUE;
      break;
    case JSON_ARRAY_START:
      Json.states[Json.encode_depth] = JSON_IN_ARRAY;
      break;
    case JSON_OBJ_VALUE:
      Json.states[Json.encode_depth] = JSON_OBJ_KEY;
      break;
    default:
      break;
  }
}

/****************************************************************************
 JsonDecode
****************************************************************************/

typedef enum {
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
} JsonDecode_Token;

struct Json_Callbacks_t
{
  bool(*null_callback)(void *ctx);
  bool(*bool_callback)(void*, bool);
  bool(*int_callback)(void*, int);
  bool(*float_callback)(void*, float);
  bool(*string_callback )(void*, char*, int);
  bool(*start_obj_callback )(void*);
  bool(*obj_key_callback )(void*, char*, int);
  bool(*end_obj_callback )(void*);
  bool(*start_array_callback )(void *ctx);
  bool(*end_array_callback )(void *ctx);
} Json_Callbacks;

static JsonDecode_Token JsonDecode_GetToken(char* text, int len);

/**
  Set the function to be called back when an integer is parsed.
  The function must have the format:
  \code
  bool on_int(void* context, int value);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetIntCallback(on_int);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param int_callback The function to be called back.
*/
void JsonDecode_SetIntCallback(bool(*int_callback)(void *ctx, int val))
{
  Json_Callbacks.int_callback = int_callback;
}

/**
  Set the function to be called back when a float is parsed.
  The function must have the format:
  \code
  bool on_float(void* context, float value);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetFloatCallback(on_float);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param float_callback The function to be called back.
*/
void JsonDecode_SetFloatCallback(bool(*float_callback)(void *ctx, float val))
{
  Json_Callbacks.float_callback = float_callback;
}

/**
  Set the function to be called back when a boolean is parsed.
  The function must have the format:
  \code
  bool on_bool(void* context, bool value);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetBoolCallback(on_bool);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param bool_callback The function to be called back.
*/
void JsonDecode_SetBoolCallback(bool(*bool_callback)(void *ctx, bool val))
{
  Json_Callbacks.bool_callback = bool_callback;
}

/**
  Set the function to be called back when a string is parsed.
  The function must have the format:
  \code
  bool on_string(void* context, char* string);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetStringCallback(on_string);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param string_callback The function to be called back.
*/
void JsonDecode_SetStringCallback(bool(*string_callback)(void *ctx, char *string, int len))
{
  Json_Callbacks.string_callback = string_callback;
}

/**
  Set the function to be called back when an object is started.
  The left bracket - { - is the opening element of an object.
  The function must have the format:
  \code
  bool on_obj_started(void* context);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetStartObjCallback(on_obj_started);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param start_obj_callback The function to be called back.
*/
void JsonDecode_SetStartObjCallback(bool(*start_obj_callback)(void *ctx))
{
  Json_Callbacks.start_obj_callback = start_obj_callback;
}

void JsonDecode_SetObjKeyCallback(bool(*obj_key_callback)(void *ctx, char *key, int len))
{
  Json_Callbacks.obj_key_callback = obj_key_callback;
}

/**
  Set the function to be called back when an object is ended.
  The right bracket - } - is the closing element of an object.
  The function must have the format:
  \code
  bool on_obj_ended(void* context);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetEndObjCallback(on_obj_ended);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param end_obj_callback The function to be called back.
*/
void JsonDecode_SetEndObjCallback(bool(*end_obj_callback)(void *ctx))
{
  Json_Callbacks.end_obj_callback = end_obj_callback;
}

/**
  Set the function to be called back when an array is started.
  The left brace - [ - is the starting element of an array.
  The function must have the format:
  \code
  bool on_array_started(void* context);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetStartArrayCallback(on_array_started);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param start_array_callback The function to be called back.
*/
void JsonDecode_SetStartArrayCallback(bool(*start_array_callback)(void *ctx))
{
  Json_Callbacks.start_array_callback = start_array_callback;
}

/**
  Set the function to be called back when an array is ended.
  The right brace - ] - is the closing element of an array.
  The function must have the format:
  \code
  bool on_array_ended(void* context);
  \endcode
  which would then be registered like so:
  \code
  JsonDecode_SetEndArrayCallback(on_array_ended);
  \endcode
  The \b context parameter can be optionally passed to JsonDecode() 
  to provide context within the callback.

  @param end_array_callback The function to be called back.
*/
void JsonDecode_SetEndArrayCallback(bool(*end_array_callback)(void *ctx))
{
  Json_Callbacks.end_array_callback = end_array_callback;
}

/**
  Parse a JSON string.
  The JSON parser is event based, meaning that you will receive any callbacks
  you registered for as the elements are encountered in the JSON string.

  @param text The JSON string to parse.
  @param len The length of the JSON string.
  @param context An optional paramter that your code can use to 
  pass around a known object within the callbacks.  Otherwise, just set it to 0
  @return True on successful parse, false on failure.

  \par Example
  \code
  // quotes are escaped since I'm writing it out manually
  char jsonstr[] = "[{\"label\":\"value\",\"label2\":{\"nested\":234}}]";
  JsonDecode(jsonstr, strlen(jsonstr), 0); // don't pass in any context
  // now we expect to be called back on any callbacks we registered.
  \endcode
*/
bool JsonDecode(char* text, int len, void* context)
{
  JsonDecode_Token token;
  typedef enum {
    JSON_DECODE_OBJECT_START,
    JSON_DECODE_IN_OBJECT,
    JSON_DECODE_IN_ARRAY
  } JsonDecode_State;

  JsonDecode_State states[JSON_MAX_DEPTH];
  int depth = 0;
  bool gotcomma = false;

  while(len)
  {
    while(*text == ' ') // eat white space
      text++;
    token = JsonDecode_GetToken( text, len);
    switch(token)
    {
      case token_true:
        if(Json_Callbacks.bool_callback)
        {
          if(!Json_Callbacks.bool_callback(context, true))
            return false;
        }
        text += 4;
        len -= 4;
        break;
      case token_false:
        if(Json_Callbacks.bool_callback)
        {
          if(!Json_Callbacks.bool_callback(context, false))
            return false;
        }
        text += 5;
        len -= 5;
        break;
      case token_null:
        if(Json_Callbacks.null_callback)
        {
          if(!Json_Callbacks.null_callback(context))
            return false;
        }
        text += 4;
        len -= 4;
        break;
      case token_comma:
        gotcomma = true;
        // intentional fall-through
      case token_colon: // just get the next one      
        text++;
        len--;
        break;
      case token_left_bracket:
        if(Json_Callbacks.start_obj_callback)
        {
          if(!Json_Callbacks.start_obj_callback(context))
            return false;
        }
        states[++depth] = JSON_DECODE_OBJECT_START;
        text++;
        len--;
        break;
      case token_right_bracket:
        if(Json_Callbacks.end_obj_callback)
        {
          if(!Json_Callbacks.end_obj_callback(context))
            return false;
        }
        depth--;
        text++;
        len--;
        break;
      case token_left_brace:
        if(Json_Callbacks.start_array_callback)
        {
          if(!Json_Callbacks.start_array_callback(context))
            return false;
        }
        states[++depth] = JSON_DECODE_IN_ARRAY;
        text++;
        len--;
        break;
      case token_right_brace:
        if(Json_Callbacks.end_array_callback)
        {
          if(!Json_Callbacks.end_array_callback(context))
            return false;
        }
        depth--;
        text++;
        len--;
        break;
      case token_number:
      {
        const char* p = text;
        bool keepgoing = true;
        bool gotdecimal = false;
        do
        {
          if(*p == '.')
          {
            if(gotdecimal) // we only expect to get one decimal place in a number
              return false;
            gotdecimal = true;
            p++;
          }
          else if(!isdigit(*p))
            keepgoing = false;
          else
            p++;
        } while(keepgoing);
        int size = p - text;
        if(gotdecimal)
        {
          if(Json_Callbacks.float_callback)
          {
            if(!Json_Callbacks.float_callback(context, atof(text)))
              return false;
          }
        }
        else
        {
          if(Json_Callbacks.int_callback)
          {
            if(!Json_Callbacks.int_callback(context, atoi(text)))
              return false;
          }
        }
        text += size;
        len -= size;
        break;
      }
      case token_string:
      {
        char* p = ++text;
        len--;
        while(*p != '"') // just go until the next " for now...may ultimately check for escaped data
          p++;
        int size = p - text;
        *p = 0; // replace the trailing " with a null to make a string

        // figure out if this is a key or a normal string
        bool objkey = false;
        if(states[depth] == JSON_DECODE_OBJECT_START)
        {
          states[depth] = JSON_DECODE_IN_OBJECT;
          objkey = true;
        }
        if(gotcomma && states[depth] == JSON_DECODE_IN_OBJECT)
        {
          gotcomma = false;
          objkey = true;
        }

        if(objkey) // last one was a comma - next string has to be a key
        {
          if(Json_Callbacks.obj_key_callback)
          {
            if(!Json_Callbacks.obj_key_callback(context, text, size))
              return false;
          }
        }
        else // just a normal string
        {
          if(Json_Callbacks.string_callback)
          {
            if(!Json_Callbacks.string_callback(context, text, size))
              return false;
          }
        }
        text += (size+1); // account for the trailing "
        len -= (size+1);
        break;
      }
      case token_eof: // we don't expect to get this, since our len should run out before we get eof
        return false;
      default:
        return false;
    }
  }
  return true;
}

// static
JsonDecode_Token JsonDecode_GetToken(char* text, int len)
{
  char c = text[0];
  switch(c)
  {
    case ':':
      return token_colon;
    case ',':
      return token_comma;
    case '{':
      return token_left_bracket;
    case '}':
      return token_right_bracket;
    case '[':
      return token_left_brace;
    case ']':
      return token_right_brace;
    case '"':
      return token_string;
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
      return token_number;
    case '-':
      return token_maybe_negative;
    case 't':
    {
      if(len < 4) // not enough space;
        return token_unknown;
      if(!strncmp(text, "true", 4))
        return token_true;
      else 
        return token_unknown;
    }
    case 'f':
    {
      if(len < 5) // not enough space;
        return token_unknown;
      if(!strncmp(text, "false", 5))
        return token_false;
      else
        return token_unknown;
    }
    case 'n':
    {
      if(len < 4) // not enough space;
        return token_unknown;
      if(!strncmp(text, "null", 4))
        return token_null;
      else
        return token_unknown;
    }
    case '\0':
      return token_eof;
    default:
      return token_unknown;
  }
}





