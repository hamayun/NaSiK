#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int
_DEFUN (_stat, (file, st),
        const char  *file _AND
        struct stat *st)
{
	st -> st_mode = S_IFCHR;
	return 0;
}

