#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#include <sys/types.h>

int
_DEFUN (_chown, (path, owner, group),
        const char *path  _AND
        uid_t owner _AND
        gid_t group)
{
  errno = ENOSYS;
  return -1;
}
