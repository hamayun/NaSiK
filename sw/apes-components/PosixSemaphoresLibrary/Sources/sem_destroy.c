#include <Private/PosixSemaphores.h>

int sem_destroy(sem_t *sem)
{
  status_t status;

  watch (int)
  {
    check (bad_argument, sem != NULL, -1);

    if ((status = semaphore_destroy(sem->id)) != DNA_OK)
    {
      errno = EINVAL;
      return -1;
    }

    return 0;
  }

  rescue (bad_argument)
  {
    errno = EINVAL;
    leave;
  }
}
