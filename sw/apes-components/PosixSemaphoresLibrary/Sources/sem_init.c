#include <Private/PosixSemaphores.h>

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
  status_t status;
  int32_t result = 0;

  if (pshared != 0) {
    result = ENOSYS;
  } else if (value > (unsigned int)SEM_VALUE_MAX) {
    result = EINVAL;
  } else if (sem == NULL) {
    result = EINVAL;
  } else {
    /* Posix says that this function should not be called more than once on
       a given semaphore, otherwise the behavior is undefined.
       So we do not take any precautions here.
    */
    if ((status = semaphore_create("", value, &sem->id)) != DNA_OK) {
      switch (status) {
      case DNA_NO_MORE_SEM:
        result = EAGAIN;
      case DNA_OUT_OF_MEM:
        result = ENOMEM;
      default:
        result = EINVAL;
      }
    }
  }

  if (result != 0) {
    errno = result;
    return -1;
  }

  return 0;
}
