#include <Private/PosixSemaphores.h>

int sem_post(sem_t *sem)
{
  status_t status;
  int32_t result = 0;
  semaphore_info_t info;

  if (sem == NULL) {
    result = EINVAL;
  } else if ((status = semaphore_get_info(sem->id, &info)) != DNA_OK) {
    result = EINVAL;
  } else if (info.tokens > SEM_VALUE_MAX) {
    result = EOVERFLOW;
  } else if ((status = semaphore_release(sem->id, 1, 0)) != DNA_OK) {
    result = EINVAL;
  }

  if (result != 0) {
    errno = result;
    return -1;
  }

  return 0;
}
