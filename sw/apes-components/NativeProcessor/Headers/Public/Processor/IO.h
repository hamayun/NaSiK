#ifndef PROCESSOR_IO_H
#define PROCESSOR_IO_H

/*
 * Write operations.
 */

extern void __dnaos_hal_write_uint8(uint8_t *addr, uint8_t value);
extern void __dnaos_hal_write_uint16(uint16_t *addr, uint16_t value);
extern void __dnaos_hal_write_uint32(uint32_t *addr, uint32_t value);

#define cpu_write(type,addr,value) cpu_write_##type(addr,value)

#define cpu_write_UINT8(addr,value)                                   \
  __dnaos_hal_write_uint8((uint8_t *)(addr),(uint8_t)(value))

#define cpu_write_UINT16(addr,value)                                  \
  __dnaos_hal_write_uint16((uint16_t *)(addr),(uint16_t)(value))

#define cpu_write_UINT32(addr,value)                                  \
  __dnaos_hal_write_uint32((uint32_t *)(addr),(uint32_t)(value))

/*
 * Read operations.
 */

extern uint8_t  __dnaos_hal_read_uint8(uint8_t *addr);
extern uint16_t __dnaos_hal_read_uint16(uint16_t *addr);
extern uint32_t __dnaos_hal_read_uint32(uint32_t *addr);

#define cpu_read(type,addr,value) cpu_read_##type(addr,value)

#define cpu_read_UINT8(addr,value)                                    \
  (value) = __dnaos_hal_read_uint8((uint8_t *)(addr))

#define cpu_read_UINT16(addr, value)                                  \
  (value) = __dnaos_hal_read_uint16((uint16_t *)(addr))

#define cpu_read_UINT32(addr,value)                                   \
  (value) = __dnaos_hal_read_uint32((uint32_t *)(addr))

/*
 * Uncached operations.
 */

#define cpu_uncached_write(type,addr,value) cpu_write(type,addr,value)
#define cpu_uncached_read(type,addr,value) cpu_read(type,addr,value)

/*
 * Vector operations
 */

extern void __dnaos_hal_vector_write_dfloat(double *to,double *from, unsigned long int len);
extern void __dnaos_hal_vector_write_sfloat(float *to,float *from, unsigned long int len); 
extern void __dnaos_hal_vector_write_uint64(uint64_t *to, uint64_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint32(uint32_t *to, uint32_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint16(uint16_t *to, uint16_t *from, unsigned long int len);
extern void __dnaos_hal_vector_write_uint8 (uint8_t *to, uint8_t *from, unsigned long int len);

#define cpu_vector_write(mode,to,from,len) cpu_vector_write_##mode(to,from,len)

#define cpu_vector_write_DFLOAT(to,from,len)     \
  __dnaos_hal_vector_write_dfloat(to,from,len)

#define cpu_vector_write_SFLOAT(to,from,len)     \
  __dnaos_hal_vector_write_sfloat(to,from,len)

#define cpu_vector_write_UINT64(to,from,len)     \
  __dnaos_hal_vector_write_uint64(to,from,len)

#define cpu_vector_write_UINT32(to,from,len)     \
  __dnaos_hal_vector_write_uint32(to,from,len)

#define cpu_vector_write_UINT16(to,from,len)     \
  __dnaos_hal_vector_write_uint16(to,from,len)

#define cpu_vector_write_UINT8(to,from,len)      \
  __dnaos_hal_vector_write_uint8(to,from,len)

#define cpu_vector_read(mode,to,from,len) \
  cpu_vector_write_##mode(to,from,len)

#endif

