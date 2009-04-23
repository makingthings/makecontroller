

#include <reent.h>
#include <stdlib.h>

#include "projdefs.h"
#include "portable.h"

/************************** _sbrk_r *************************************
  Support function. Adjusts end of heap to provide more memory to
  memory allocator. Simple and dumb with no sanity checks.
  
  This was originally added to provide floating point support
  in printf and friends.
 
  struct _reent *r -- re-entrancy structure, used by newlib to
                       support multiple threads of operation.
  ptrdiff_t nbytes -- number of bytes to add.
                       Returns pointer to start of new heap area.
 
   Note:  This implementation is not thread safe (despite taking a
          _reent structure as a parameter).
          Since _s_r is not used in the current implementation, 
          the following messages must be suppressed.
  
  Provided by Uli Hartmann.
  
  References:
    http://www.embedded.com/columns/15201376?_requestid=243242
    http://forum.sparkfun.com/viewtopic.php?t=5390&postdays=0&postorder=asc&start=0
    http://www.siwawi.arubi.uni-kl.de/avr_projects/arm_projects/index_at91.html
*/

/*
  'end' is set in the linker command file and is the end of statically
  allocated data (thus start of heap).
*/
extern char end[];
static char *heap_ptr; // Points to current end of the heap
void* _sbrk_r(struct _reent *r, ptrdiff_t nbytes)
{
  (void)r;
  char *base;         //  errno should be set to  ENOMEM on error

  if (!heap_ptr)      //  Initialize if first time through.
    heap_ptr = end;
  base = heap_ptr;    //  Point to end of heap.
  heap_ptr += nbytes; //  Increase heap.
  return base;        //  Return pointer to start of new heap area.
}

int _open_r (struct _reent *r, const char *buf, int flags, int mode)
{
  (void) r;
  (void) buf;
  (void) flags;
  (void) mode;
  return -1;
}
//int isatty( int fd ) { return -1; }
_off_t _lseek_r ( struct _reent *ptr, int fd, _off_t offset, int whence )
{
  (void) ptr;
  (void) fd;
  (void) offset;
  (void) whence;
  return -1;
}
_ssize_t _write_r (struct _reent *r, int fd, const void *buf, size_t nbytes)
{
  (void) r;
  (void) fd;
  (void) buf;
  (void) nbytes;
  return -1;
}
int _close_r (struct _reent *r, int fd)
{
  (void) r;
  (void) fd;
  return -1;
}
_ssize_t _read_r (struct _reent *r, int fd, void *buf, size_t nbytes)
{
  (void) r;
  (void) fd;
  (void) buf;
  (void) nbytes;
  return -1;
}
int _fstat_r (struct _reent *r, int fd, struct stat *buf)
{
  (void) r;
  (void) fd;
  (void) buf;
  return -1;
}

int _kill_r(struct _reent* r, int pid, int sig)
{
  (void) r;
  (void) pid;
  (void) sig;
  return -1;
}

void* malloc( size_t size )
{
  return pvPortMalloc( size );
}

void free( void* memory )
{
  vPortFree( memory );
}


