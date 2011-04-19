#include <_ansi.h>
#include <_syslist.h>

#include <stdint.h>
#include <stddef.h>

extern uint8_t * OS_USER_HEAP_ADDRESS;

void *
_DEFUN (_sbrk, (incr),
     int incr)
{ 
  uint8_t * ptr_start = NULL;
  
  ptr_start = (uint8_t *)(((uint32_t)(OS_USER_HEAP_ADDRESS + 8)) & 0xFFFFFFF8);
  OS_USER_HEAP_ADDRESS = OS_USER_HEAP_ADDRESS + incr; 

  return ptr_start;
} 
