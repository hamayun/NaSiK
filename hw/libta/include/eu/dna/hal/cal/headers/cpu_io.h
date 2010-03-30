#ifndef _NATIVE_CPU_IO_H_
#define _NATIVE_CPU_IO_H_

#include "stdint.h"

extern void __dnaos_hal_write_uint8(uint8_t *addr, uint8_t value);
extern void __dnaos_hal_write_uint16(uint16_t *addr, uint16_t value);
extern void __dnaos_hal_write_uint32(uint32_t *addr, uint32_t value);
extern uint8_t  __dnaos_hal_read_uint8(uint8_t *addr);
extern uint16_t __dnaos_hal_read_uint16(uint16_t *addr);
extern uint32_t __dnaos_hal_read_uint32(uint32_t *addr);
extern void __dnaos_hal_vector_write_dfloat(double *to,double *from, unsigned long int len);
extern void __dnaos_hal_vector_write_sfloat(float *to,float *from, unsigned long int len); 
extern void __dnaos_hal_vector_write_uint64(uint64_t *to, uint64_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint32(uint32_t *to, uint32_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint16(uint16_t *to, uint16_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint8 (uint8_t *to, uint8_t *from, unsigned long int len);

#define CPU_WRITE(type,addr,value) CPU_WRITE_##type(addr,value)
#define CPU_WRITE_UINT8(addr,value)  __dnaos_hal_write_uint8((uint8_t *)(addr),(value))
#define CPU_WRITE_UINT16(addr,value) __dnaos_hal_write_uint16((uint16_t *)(addr),(value))
#define CPU_WRITE_UINT32(addr,value) __dnaos_hal_write_uint32((uint32_t *)(addr),(value))

#define CPU_READ(type,addr,value) CPU_READ_##type(addr,value)
#define CPU_READ_UINT8(addr,value) (value) =   __dnaos_hal_read_uint8((uint8_t *)(addr))
#define CPU_READ_UINT16(addr, value) (value) = __dnaos_hal_read_uint16((uint16_t *)(addr))
#define CPU_READ_UINT32(addr,value) (value) =  __dnaos_hal_read_uint32((uint32_t *)(addr))

#define CPU_UNCACHED_WRITE(type,addr,value) CPU_WRITE(type,addr,value)
#define CPU_UNCACHED_READ(type,addr,value) CPU_READ(type,addr,value)

#define CPU_VECTOR_WRITE(mode,to,from,len) CPU_VECTOR_WRITE_##mode(to,from,len)

#define CPU_VECTOR_WRITE_DFLOAT(to,from,len) __dnaos_hal_vector_write_dfloat(to,from,len)
#define CPU_VECTOR_WRITE_SFLOAT(to,from,len) __dnaos_hal_vector_write_sfloat(to,from,len) 
#define CPU_VECTOR_WRITE_UINT64(to,from,len) __dnaos_hal_vector_write_uint64(to,from,len) 
#define CPU_VECTOR_WRITE_UINT32(to,from,len) __dnaos_hal_vector_write_uint32(to,from,len) 
#define CPU_VECTOR_WRITE_UINT16(to,from,len) __dnaos_hal_vector_write_uint16(to,from,len) 
#define CPU_VECTOR_WRITE_UINT8(to,from,len)  __dnaos_hal_vector_write_uint8(to,from,len) 
#define CPU_VECTOR_READ(mode,to,from,len) CPU_VECTOR_WRITE_##mode(to,from,len)

#endif


