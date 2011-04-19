#include <stdlib.h>
#include <malloc.h>

#include <Core/Core.h>

extern int32_t __sanity_sem;
extern int __libthread_start (void);

int _main(void)
{
  /*
   * Create the sanity check semaphore
   */

  semaphore_create ("libc_sanity_sem", 1, &__sanity_sem);

  /*
   * Return from the application's call
   */

	return __libthread_start ();
}

