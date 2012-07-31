#include <Private/PosixSemaphores.h>

int sem_wait(sem_t *sem)
{
  status_t status;
  int32_t result = 0;

  if (sem == NULL) {
    result = EINVAL;
  }

  if ((status = semaphore_acquire(sem->id, 1, 0, -1)) != DNA_OK) {
    result = EINVAL;
  }

  if (result != 0) {
    errno = result;
    return -1;
  }
  
  return 0;
}
