
/*
  In order to keep the impact of linking to C++ to a minimum, 
  redefine the new and delete operators to functions we already
  have via the RTOS.
  
  As described in the series of articles starting with 
  http://www.embedded.com/design/opensource/200000632?_requestid=257225
*/


#include <stdlib.h>

extern "C" {
  #include "projdefs.h"
  #include "portable.h"
}

void* __dso_handle = (void*) &__dso_handle;

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

extern "C" void __cxa_pure_virtual(void)
{
  /*
    This should never get called in normal operation, but does get referenced by compiled C++ code.
    The only time this function should really be called is if a virtual function is called while the 
    object is still being created, which gives undefined behaviour.
    If it's deemed necessary, we can put some sort of debug utility here to indicate that some badness has happened.
  */
}




