#ifndef TG_H
#define TG_H

#include <stdint.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <libos/libos.h>
#include <unistd.h>
#include <fcntl.h>
#include <cpu.h>

#include "mjpeg.h"
#include "utils.h"

enum fdaccess_control
{
  FD_OPEN = DNA_CONTROL_CODES_END,
  FD_CLOSE,
  FD_LSEEK
};

/*---- Usefull macros ----*/
#if 0
#define NEXT_TOKEN(res) res = (p -> local -> movie[p -> local -> index++])

#define COPY_SECTION(to, size) {																															\
	memcpy ((void *)to), (const void *)(& (p -> local -> movie[p -> local -> index])), size);	\
	p -> local -> index += size;																																\
}

#define SKIP(n) p -> local -> index += n

#define MOVIE_DATA										\
	unsigned long int index;						\
	unsigned char * movie

#define INITIALIZE_MOVIE_DATA					\
	p -> local -> index = 0;						\
	p -> local -> movie = movie

#else

#define NEXT_TOKEN(res) { 							\
	read (movie, (void *) & res, 1); 	\
}

#define COPY_SECTION(to, size) { \
	read (movie, (void *) to, size); \
}

#define SKIP(n) {																																\
	uint8_t waste[2048];																													\
																																								\
	if (n != 0) read (movie, (void *) & waste, n);							\
}

#define MOVIE_DATA int16_t movie

#define INITIALIZE_MOVIE_DATA																\
  int32_t retval = 0;                     \
	movie = open ("/devices/fdaccess", O_RDWR);  \
  vfs_component . operation . ioctl (movie, FD_OPEN, "movie.mjpeg", & retval);\
  if (retval < 0) printf ("Error opening the video file.\r\n");
#endif

#endif
