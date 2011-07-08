
#ifndef PROCESSOR_PROFILE_H
#define PROCESSOR_PROFILE_H

#define ENABLE_PROFILING
extern short HOSTTIME_BASEPORT;

typedef enum hosttime_port_offset
{
    HOSTTIME_CURRENT_TIME = 0,
    HOSTTIME_COMP_START = 1,
    HOSTTIME_COMP_END = 2,
    HOSTTIME_IO_START = 3,
    HOSTTIME_IO_END = 4,
    HOSTTIME_FLUSH_DATA = 5
} hosttime_port_t;

#ifdef ENABLE_PROFILING
#define CPU_PROFILE_CURRENT_TIME()                              \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_CURRENT_TIME)      \
            :"%dx"                                              \
        );                                                      \
    } while (0)

#define CPU_PROFILE_COMP_START()                                \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_COMP_START)        \
            :"%dx"                                              \
        );                                                      \
    } while (0)

#define CPU_PROFILE_COMP_END()                                  \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_COMP_END)          \
            :"%dx"                                              \
        );                                                      \
    } while (0)

#define CPU_PROFILE_IO_START()                                  \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_IO_START)          \
            :"%dx"                                              \
        );                                                      \
    } while (0)

#define CPU_PROFILE_IO_END()                                    \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_IO_END)            \
            :"%dx"                                              \
        );                                                      \
    } while (0)

#define CPU_PROFILE_FLUSH_DATA()                                \
    do{                                                         \
        __asm__ volatile(                                       \
            "   mov  $0x5000,%%dx\n\t"                               \
            "   mov  %1,%%eax\n\t"                              \
            "   out  %%eax,(%%dx)\n\t"                          \
            ::"r"((short int) HOSTTIME_BASEPORT),               \
              "r"((hosttime_port_t) HOSTTIME_FLUSH_DATA)        \
            :"%dx"                                              \
        );                                                      \
    } while (0)
#else
#define CPU_PROFILE_CURRENT_TIME()
#define CPU_PROFILE_COMP_START()
#define CPU_PROFILE_COMP_END()
#define CPU_PROFILE_IO_START()
#define CPU_PROFILE_IO_END()
#define CPU_PROFILE_FLUSH_DATA()
#endif /* ENABLE_PROFILING */

#endif	/* PROCESSOR_PROFILE_H */

