#include <_ansi.h>
#include <_syslist.h>
#include <dirent.h>

#include <VirtualFileSystem/VirtualFileSystem.h>

int
_DEFUN (getdents, (fd, dirp, count),
        unsigned int   fd  _AND
        struct dirent *dirp   _AND
        unsigned int count)
{
	status_t status = DNA_OK;
	int32_t res = -1;

	status = vfs_readdir (fd, (directory_entry_t *)dirp, count, & res);
	return res;
}

