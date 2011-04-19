#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_link, (existing, new),
        char *existing _AND
        char *new)
{
  errno = ENOSYS;
  return -1;
}

