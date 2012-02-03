
/* the flush_write_buffers function is taken from the linux realization*/
//#if defined(CONFIG_X86_OOSTORE) || defined(CONFIG_X86_PPRO_FENCE)

static inline void flush_write_buffers(void)
{
	//asm volatile("lock; addl $0,0(%%esp)": : :"memory");
}

//#else
//
//#define flush_write_buffers() do { } while (0)
//
//#endif

//static inline void cpu_cache_sync(void)      
//{                             
//	flush_write_buffers();
//}


void cpu_cache_sync(void)      
{                             
	flush_write_buffers();
}


