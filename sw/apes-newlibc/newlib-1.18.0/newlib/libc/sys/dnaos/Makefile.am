## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS) -I$(srcdir)/include

AM_CCASFLAGS = $(INCLUDES)

noinst_LIBRARIES = lib.a

lib_a_SOURCES = chown.c close.c getdents.c\
	environ.c errno.c execve.c _exit.c fork.c fstat.c \
	getpid.c gettod.c impure.c isatty.c ioctl.c kill.c lock.c link.c \
	lseek.c nanosleep.c open.c read.c readlink.c \
	sbrk.c stat.c symlink.c times.c unlink.c wait.c write.c

lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)

all: crt0.o

ACLOCAL_AMFLAGS = -I ../../.. -I ../../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host
