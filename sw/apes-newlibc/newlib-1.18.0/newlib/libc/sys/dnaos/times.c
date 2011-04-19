#include <_ansi.h>
#include <_syslist.h>
#include <stddef.h>
#include <sys/times.h>

#include <Core/Core.h>

clock_t
_DEFUN (_times, (buf),
        struct tms *buf)
{
	int32_t thread;
	thread_info_t t_info;
  status_t status = DNA_OK;
	
  if (buf != NULL)
  {
    status = thread_find (NULL, & thread);
    thread_get_info (thread, & t_info);

    buf -> tms_utime = t_info . user_time;
    buf -> tms_stime = t_info . kernel_time;
    buf -> tms_cutime = 0;
    buf -> tms_cstime = 0;
  }

	return 0;
}

