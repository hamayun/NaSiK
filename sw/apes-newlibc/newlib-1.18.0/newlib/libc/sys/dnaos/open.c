#include <_ansi.h>
#include <_syslist.h>

#include <VirtualFileSystem/VirtualFileSystem.h>

int
_DEFUN (_open, (file, flags, mode),
        char *file  _AND
        int   flags _AND
        int   mode)
{
	int16_t fd = -1;
	status_t status = DNA_OK;

	status = vfs_open (file, flags, mode, & fd);
	return fd;
}

