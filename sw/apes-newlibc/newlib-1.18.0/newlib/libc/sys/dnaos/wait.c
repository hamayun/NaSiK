#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_wait, (status),
        int  *status)
{
  errno = ENOSYS;
  return -1;
}

