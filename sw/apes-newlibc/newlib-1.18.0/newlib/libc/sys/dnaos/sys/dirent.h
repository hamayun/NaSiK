#ifndef _SYS_DIRENT_H
# define _SYS_DIRENT_H

#include <sys/types.h>

typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	char	*dd_buf;
	int	dd_len;
	long	dd_seek;
} DIR;

# define __dirfd(dp)	((dp)->dd_fd)

extern DIR *opendir (const char *);
extern struct dirent *readdir (DIR *);
extern void rewinddir (DIR *);
extern int closedir (DIR *);
extern long telldir (DIR *);
extern void seekdir (DIR *, long loc);

struct dirent {
	ino_t	d_ino;
	dev_t	d_dev;
	unsigned short d_reclen;
	char d_name[1];
};

#endif
