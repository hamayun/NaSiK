#include <_ansi.h>
#include <_syslist.h>
#include <stddef.h>
#include <errno.h>

#include <Core/Core.h>

volatile int32_t __sanity_sem = -1;

void
_DEFUN (_dna_lock_init, (lock),
        dna_lock_t * lock)
{
  semaphore_acquire (__sanity_sem, 1, 0, -1);

  semaphore_create ("libc_mutex", 1, & lock -> id);
  lock -> count = 0;

  semaphore_release (__sanity_sem, 1, DNA_NO_RESCHEDULE);
}

void
_DEFUN (_dna_lock_close, (lock),
        dna_lock_t * lock)
{
  semaphore_acquire (__sanity_sem, 1, 0, -1);

  if (lock -> id != -1)
  {
    semaphore_destroy (lock -> id);
    lock -> id = -1;
    lock -> count = 0;
  }

  semaphore_release (__sanity_sem, 1, DNA_NO_RESCHEDULE);
}

void
_DEFUN (_dna_lock_acquire, (lock),
        dna_lock_t * lock)
{
  int32_t thread_id = -1;
  semaphore_info_t sem_info;
  status_t status = DNA_OK;

  /*
   * We need to check the validity of the lock
   */

  semaphore_acquire (__sanity_sem, 1, 0, -1);

  if (lock -> id == -1)
  {
    semaphore_create ("libc_mutex_static", 1, & lock -> id);
    lock -> count = 0;
  }

  semaphore_release (__sanity_sem, 1, DNA_NO_RESCHEDULE);

  /*
   * And then we proceed
   */

  thread_find (NULL, & thread_id);

  status = semaphore_acquire (lock -> id, 1, DNA_RELATIVE_TIMEOUT, 0);
  semaphore_get_info (lock -> id, & sem_info);

  if (status == DNA_WOULD_BLOCK)
  {
    if (sem_info . latest_holder != thread_id || lock -> count == 0)
    {
      semaphore_acquire (lock -> id, 1, 0, -1);
    }
  }

  /*
   * Increase the usage counter.
   */

  lock -> count += 1;
}

void
_DEFUN (_dna_lock_release, (lock),
        dna_lock_t * lock)
{
  if (lock -> id != -1)
  {
    lock -> count -= 1;

    if (lock -> count == 0)
    {
      semaphore_release (lock -> id, 1, DNA_NO_RESCHEDULE);
    }
  }
}

