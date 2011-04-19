#include <_ansi.h>
#include <_syslist.h>
#include <stdarg.h>
#include <VirtualFileSystem/VirtualFileSystem.h>

int ioctl (int fd, int request,...)
{
    va_list ap;
    int32_t res;

    va_start (ap, request);
    vfs_ioctl (fd, request, ap, & res);
    va_end (ap);

    return res;
}

