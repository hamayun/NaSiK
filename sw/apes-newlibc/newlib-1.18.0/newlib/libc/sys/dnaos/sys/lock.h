#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

#include <_ansi.h>
#include <stdint.h>

typedef struct _dna_lock
{
  int32_t id;
  int32_t count;
}
dna_lock_t;

typedef dna_lock_t _LOCK_T;
typedef dna_lock_t _LOCK_RECURSIVE_T;
 
extern void _dna_lock_init (dna_lock_t * lock);
extern void _dna_lock_close (dna_lock_t * lock);
extern void _dna_lock_acquire (dna_lock_t * lock);
extern void _dna_lock_release (dna_lock_t * lock);


#define __LOCK_INIT(class,lock) class dna_lock_t lock = { -1, 0 };
#define __LOCK_INIT_RECURSIVE(class,lock) class dna_lock_t lock = { -1, 0 };

#define __lock_init(lock) _dna_lock_init (& lock)
#define __lock_init_recursive(lock) _dna_lock_init (& lock)

#define __lock_close(lock) _dna_lock_close (& lock)
#define __lock_close_recursive(lock) _dna_lock_close (& lock)

#define __lock_acquire(lock) _dna_lock_acquire (& (lock))
#define __lock_acquire_recursive(lock) _dna_lock_acquire (& (lock))

#define __lock_try_acquire(lock) (_CAST_VOID 0)
#define __lock_try_acquire_recursive(lock) (_CAST_VOID 0)

#define __lock_release(lock) _dna_lock_release (& (lock))
#define __lock_release_recursive(lock) _dna_lock_release (& (lock))

#endif /* __SYS_LOCK_H__ */
