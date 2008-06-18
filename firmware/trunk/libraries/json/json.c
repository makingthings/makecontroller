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
	The Make Controller JSON library provides a simple, and not particularly full featured, library for parsing and 
  generating JSON.  From http://www.json.org: "JSON (JavaScript Object Notation) is a lightweight 
  data-interchange format. It is easy for humans to read and write. It is easy for machines 
  to parse and generate."

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

  Note that calls to JsonEncode_String(), JsonEncode_Int() and JsonEncode_Bool() will add the
  appropriate separators (: or , usually) to the string, depending on the context of the objects 
  and arrays you've opened, or other data you've inserted.

  \par Parsing
  Not implemented yet...

  Thanks to YAJL (http://code.google.com/p/yajl-c) for some design inspiration.
  \par

	\ingroup Libraries
	@{
*/

/**
  Reset the internal state of the JSON system.
  Generally do this once you've successfully generated a string,
  and want to start on the next one.
  @param jsonbuf A pointer to the buffer you've been using to build your JSON string.
*/
void Json_Reset(char *jsonbuf)
{
  *jsonbuf = 0;
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
  if(*remaining < 1)
    return NULL;
  if(++Json.encode_depth > JSON_MAX_DEPTH)
    return NULL;
  strncat(buf, "{", 1);
  Json.states[Json.encode_depth] = JSON_OBJ_START;
  (*remaining)--;
  return buf + 1;
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
  strncat(buf, "}", 1);
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
  if(*remaining < 1)
    return NULL;
  if(++Json.encode_depth > JSON_MAX_DEPTH)
    return NULL;
  strncat(buf, "[", 1);
  Json.states[Json.encode_depth] = JSON_ARRAY_START;
  (*remaining)--;
  return buf + 1;
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
  strncat(buf, "]", 1);
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
      strncat(buf, temp, string_len);
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
      strncat(buf, temp, string_len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      string_len += 1; // for :
      if(*remaining < string_len)
        return NULL;
      char temp[string_len+1];
      snprintf(temp, string_len+1, ":\"%s\"", string);
      strncat(buf, temp, string_len);
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
      strncat(buf, temp, int_len);
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
      strncat(buf, temp, int_len);
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
      strncat(buf, temp, int_len);
      int_len = strlen(temp);
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
      strncat(buf, temp, bool_len);
      break;
    }
    case JSON_IN_ARRAY:
    {
      bool_len += 1; // for ,
      if(*remaining < bool_len)
        return NULL;
      char temp[bool_len+1];
      snprintf(temp, bool_len+1, ",%s", boolval);
      strncat(buf, temp, bool_len);
      break;
    }
    case JSON_OBJ_VALUE:
    {
      bool_len += 1; // for :
      if(*remaining < bool_len)
        return NULL;
      char temp[bool_len+1];
      snprintf(temp, bool_len+1, ":%s", boolval);
      strncat(buf, temp, bool_len);
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





