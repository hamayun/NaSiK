#include <_ansi.h>
#include <_syslist.h>
#include <sys/types.h>

#include <Core/Core.h>

int
_DEFUN(nanosleep, (rqtp, rmtp),
		const struct timespec  *rqtp _AND
	 	struct timespec *rmtp)
{
	status_t status = DNA_OK;
	uint64_t time = rqtp -> tv_nsec;

	time /= 1000;
	status = thread_snooze (time);
	return 0;
}

