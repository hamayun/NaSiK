#include <limits.h>
#include <_ansi.h>
#include <_syslist.h>

#include <Core/Core.h>

_VOID
_DEFUN (_exit, (rc),
	int rc)
{
	thread_exit (rc);

  /* Convince GCC that this function never returns.  */
  for (;;)
    ;
}
