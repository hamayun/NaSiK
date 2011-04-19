#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_kill, (pid, sig),
        int pid  _AND
        int sig)
{
  errno = ENOSYS;
  return -1;
}

