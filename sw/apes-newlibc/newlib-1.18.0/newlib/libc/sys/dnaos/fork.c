#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_fork, (),
        _NOARGS)
{
  errno = ENOSYS;
  return -1;
}

