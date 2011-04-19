#include <_ansi.h>
#include <_syslist.h>

#include <VirtualFileSystem/VirtualFileSystem.h>

int
_DEFUN (_close, (fildes),
        int fildes)
{
	status_t status = DNA_OK;

  status = vfs_close (fildes);

	if (status == DNA_OK) return 0;
	else return -1;
}

