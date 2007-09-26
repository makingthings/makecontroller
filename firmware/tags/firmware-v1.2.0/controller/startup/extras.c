

#include <reent.h>

void *_sbrk_r (struct _reent *r, ptrdiff_t t )
{
	(void)r;
	(void)t;
	return 0;
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


