#include <errno.h>

#ifndef _REENT_ONLY

extern int * __libthread__errno (void);

int *
__errno ()
{
  return __libthread_errno ();
}

#endif
