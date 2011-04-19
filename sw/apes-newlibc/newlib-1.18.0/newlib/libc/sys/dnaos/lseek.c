#include <_ansi.h>
#include <errno.h>

#include <stdint.h>
#include <sys/types.h>

#include <VirtualFileSystem/VirtualFileSystem.h>

off_t
_DEFUN (lseek, (file, off, dir),
        int   file  _AND
        off_t   off   _AND
        int   dir)
{
	off_t res = -1;
	status_t status = DNA_OK;

	status = vfs_lseek (file, off, dir, & res);
  return res;
}

