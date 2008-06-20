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

#ifndef JSON_H
#define JSON_H

#include "types.h"

// Encode
void JsonEncode_Reset(void);
char* JsonEncode_ObjectOpen(char *buf, int *remaining);
char* JsonEncode_ObjectKey(char *buf, const char *key, int *remaining);
char* JsonEncode_ObjectClose(char *buf, int *remaining);
char* JsonEncode_ArrayOpen(char *buf, int *remaining);
char* JsonEncode_ArrayClose(char *buf, int *remaining);
char* JsonEncode_String(char *buf, const char *string, int *remaining);
char* JsonEncode_Int(char *buf, int value, int *remaining);
char* JsonEncode_Bool(char *buf, bool value, int *remaining);

// Decode
void JsonDecode_SetIntCallback(bool(*int_callback)(void *ctx, int val));
void JsonDecode_SetFloatCallback(bool(*float_callback)(void *ctx, float val));
void JsonDecode_SetBoolCallback(bool(*bool_callback)(void *ctx, bool val));
void JsonDecode_SetStringCallback(bool(*string_callback)(void *ctx, char *string, int len));
void JsonDecode_SetStartObjCallback(bool(*start_obj_callback)(void *ctx));
void JsonDecode_SetObjKeyCallback(bool(*obj_key_callback)(void *ctx, char *key, int len));
void JsonDecode_SetEndObjCallback(bool(*end_obj_callback)(void *ctx));
void JsonDecode_SetStartArrayCallback(bool(*start_array_callback)(void *ctx));
void JsonDecode_SetEndArrayCallback(bool(*end_array_callback)(void *ctx));
bool JsonDecode(char* text, int len, void* context);

#endif /* JSON_H */


