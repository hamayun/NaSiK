#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>

int
_DEFUN (_execve, (name, argv, env),
        char  *name  _AND
        char **argv  _AND
        char **env)
{
  errno = ENOSYS;
  return -1;
}

