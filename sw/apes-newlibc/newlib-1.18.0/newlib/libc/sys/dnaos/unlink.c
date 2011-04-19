#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_unlink, (name),
        char *name)
{
  errno = ENOSYS;
  return -1;
}

