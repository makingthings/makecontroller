


#ifndef SCHEMA_H
#define SCHEMA_H

#include "config.h"
#include "types.h"

#define NONE {0}

typedef enum SchemaDataType_t {
  UNDEF, // uninitialized
  INT,
  FLOAT,
  STRING,
  BLOB,
  BOOLEAN
} SchemaDataType;

typedef enum SchemaOperation_t {
  GET,
  SET,
  DELETE
} SchemaOperation;

typedef struct SchemaData_t {
  SchemaDataType type;
  union {
    int i;
    float f;
    char* s;
    char* b;
    bool boolean;
  };
} SchemaData;

typedef bool (*SchemaEndpoint)(const char** uri, short idx, SchemaOperation op, SchemaData data[], int datacount);

// childNodes members are flexible length arrays.
// should ideally be declared static const so
// they're located in read-only storage.
typedef struct SchemaNode_t {
  const char* name;
  SchemaEndpoint endpoint;
  short indexCount;
  short indexOffset;
  short childCount;
  const struct SchemaNode_t* childNodes[];
} SchemaNode;

typedef struct SchemaRootNode_t {
  short childCount;
  const SchemaNode* childNodes[];
} SchemaRootNode;

// to be defined in the user app
extern const SchemaRootNode schemaRootNode;

#endif // SCHEMA_H



