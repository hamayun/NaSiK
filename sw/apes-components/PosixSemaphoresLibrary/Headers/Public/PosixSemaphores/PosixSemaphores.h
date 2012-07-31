#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

#include <Core/Core.h>

/*************************
 * Types and definitions *
 *************************/

#define SEM_VALUE_MAX	128

typedef struct _sem_t {
  int32_t	id;  /* comes from the semaphore_create function */
} sem_t;

/***********
 * METHODS *
 ***********/

extern int sem_init(sem_t *, int, unsigned int);
extern int sem_destroy(sem_t *);
extern int sem_wait(sem_t *);
//extern int sem_trywait(sem_t *);
extern int sem_post(sem_t *);
//extern int sem_getvalue(sem_t *, int *);
#endif
