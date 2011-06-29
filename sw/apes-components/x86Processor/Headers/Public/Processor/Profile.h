
#ifndef PROCESSOR_PROFILE_H
#define PROCESSOR_PROFILE_H

#define ENABLE_PROFILING

extern volatile int * HOSTTIME_REG_BASE;
extern volatile int * HOSTTIME_REG_COMP_START;
extern volatile int * HOSTTIME_REG_COMP_END;
extern volatile int * HOSTTIME_REG_IO_START;
extern volatile int * HOSTTIME_REG_IO_END;
extern volatile int * HOSTTIME_REG_FLUSH;

#ifdef ENABLE_PROFILING
#define CPU_PROFILE_CURRENT_TIME()                      \
    do{                                                 \
        *HOSTTIME_REG_BASE = 1;                         \
    } while (0)

#define CPU_PROFILE_COMP_START()                        \
    do{                                                 \
        *HOSTTIME_REG_COMP_START = 1;                   \
    } while (0)

#define CPU_PROFILE_COMP_END()                          \
    do{                                                 \
        *HOSTTIME_REG_COMP_END = 1;                     \
    } while (0)

#define CPU_PROFILE_IO_START()                          \
    do{                                                 \
        *HOSTTIME_REG_IO_START = 1;                     \
    } while (0)

#define CPU_PROFILE_IO_END()                            \
    do{                                                 \
        *HOSTTIME_REG_IO_END = 1;                       \
    } while (0)

#define CPU_PROFILE_FLUSH_DATA()                        \
    do{                                                 \
        *HOSTTIME_REG_FLUSH = 1;                        \
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

