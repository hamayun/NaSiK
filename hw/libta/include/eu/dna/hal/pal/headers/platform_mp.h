#ifndef _SOCLIB_PLATFORM_MP_H_
#define _SOCLIB_PLATFORM_MP_H_

extern unsigned int PLATFORM_N_ARM966ES;
extern unsigned int PLATFORM_N_ARM7TDMI;
extern unsigned int PLATFORM_N_SPARCV8;
extern unsigned int PLATFORM_N_MIPSR3000;
extern unsigned int PLATFORM_N_MIPSMIPS32;
extern unsigned int PLATFORM_N_NIOS2FAST;
extern unsigned int PLATFORM_N_UBLAZE81i;
extern unsigned int PLATFORM_N_NATIVE;

#define PLATFORM_MP_CPU_COUNT(what) PLATFORM_N_##what

#endif
