

/*
  In order to keep the impact of linking to C++ to a minimum, 
  redefine the new and delete operators, and 
  
  As described in the series of articles starting with 
  http://www.embedded.com/design/opensource/200000632?_requestid=257225
*/


#include <stdlib.h>

extern "C" {
  #include "projdefs.h"
  #include "portable.h"
}

void * operator new (size_t  size) throw()
{
  return pvPortMalloc( size );
}


void operator delete (void* p) throw()
{
  vPortFree(p);
}

extern "C" int __aeabi_atexit(void *object,
                              void (*destructor)(void *),
                              void *dso_handle)
{
  (void)object;
  (void)destructor;
  (void)dso_handle;
  return 0;
}