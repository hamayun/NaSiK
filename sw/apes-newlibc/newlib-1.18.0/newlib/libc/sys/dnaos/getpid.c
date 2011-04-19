#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_getpid, (),
        _NOARGS)
{
  errno = ENOSYS;
  return -1;
}

