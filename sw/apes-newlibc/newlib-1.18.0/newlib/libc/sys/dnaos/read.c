#include <_ansi.h>
#include <_syslist.h>

#include <VirtualFileSystem/VirtualFileSystem.h>

int
_DEFUN (_read, (file, ptr, len),
        int   file  _AND
        char *ptr   _AND
        int   len)
{
	status_t status = DNA_OK;
	int32_t res = -1;

	status = vfs_read (file, ptr, len, & res);
	return res;
}

