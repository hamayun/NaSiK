#include <libkvm.h>
#include <fake-apic.h>
#include <ioram.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <getopt.h>
#include <stdbool.h>

#include "iotable.h"
#include <stdint.h>
#include <sys/stat.h>
#include <libelf.h>
#include <gelf.h>
#include "soc_kvm.h"
#include "hosttime.h"

#define ERR -1

soc_kvm_data soc_kvm_init_data;

extern void * p_kvm_cpu_adaptor[];
uint64_t systemc_kvm_read_memory (void *_this, uint64_t addr, int nbytes, uint32_t *ns, int bIO);
void systemc_kvm_write_memory (void *_this, uint64_t addr, unsigned char *data, int nbytes, uint32_t *ns, int bIO);
void systemc_annotate_function(void *_this, void *vm_addr, void *pdesc);

static char *section_copy[] = {".init", ".text", ".data", ".rodata", ".rodata.str1.1", ".os_config", ".hal", ".note", ""};
static char *section_bss = {".bss"};

static int gettid(void)
{
	return syscall(__NR_gettid);
}

static int tkill(int pid, int sig)
{
	return syscall(__NR_tkill, pid, sig);
}

kvm_context_t kvm;

#define IPI_SIGNAL (SIGRTMIN + 4)

static int ncpus = 1;
static sem_t init_sem;
static __thread int vcpu;
static int apic_ipi_vector = 0xff;
static sigset_t kernel_sigmask;
static sigset_t ipi_sigmask;
static uint64_t memory_size = 128 * 1024 * 1024;

static hosttime_t hosttime_instance;
static struct io_table pio_table;

struct vcpu_info {
	int id;
	pid_t tid;
	sem_t sipi_sem;
};

struct vcpu_info *vcpus;

static uint32_t apic_sipi_addr;

static void apic_send_sipi(int vcpu)
{
	sem_post(&vcpus[vcpu].sipi_sem);
}

static void apic_send_ipi(int vcpu)
{
	struct vcpu_info *v;

	if (vcpu < 0 || vcpu >= ncpus)
		return;
	v = &vcpus[vcpu];
	tkill(v->tid, IPI_SIGNAL);
}

static int apic_io(void *opaque, int size, int is_write,
		   uint64_t addr, uint64_t *value)
{
	if (!is_write)
		*value = -1u;

	switch (addr - APIC_BASE) {
	case APIC_REG_NCPU:
		if (!is_write)
                    *value = ncpus;
		break;
	case APIC_REG_ID:
		if (!is_write)
                    *value = vcpu;
		break;
	case APIC_REG_SIPI_ADDR:
		if (!is_write)
			*value = apic_sipi_addr;
		else
			apic_sipi_addr = *value;
		break;
	case APIC_REG_SEND_SIPI:
		if (is_write)
			apic_send_sipi(*value);
		break;
	case APIC_REG_IPI_VECTOR:
		if (!is_write)
			*value = apic_ipi_vector;
		else
			apic_ipi_vector = *value;
		break;
	case APIC_REG_SEND_IPI:
		if (is_write)
			apic_send_ipi(*value);
		break;
	}

	return 0;
}

static int apic_init(void)
{
	return io_table_register(&pio_table, APIC_BASE,
				 APIC_SIZE, apic_io, NULL);
}

static int misc_io(void *opaque, int size, int is_write,
		   uint64_t addr, uint64_t *value)
{
	static int newline = 1;

	if (!is_write)
		*value = -1;

	switch (addr) {
	case 0xff: // irq injector
		if (is_write) {
			printf("injecting interrupt 0x%x\n", (uint8_t)*value);
			kvm_inject_irq(kvm, 0, *value);
		}
		break;
	case 0xf1: // serial
		if (is_write) {
			if (newline)
				fputs("GUEST: ", stdout);
			putchar(*value);
			newline = *value == '\n';
		}
		break;
	case 0xd1:
		if (!is_write)
			*value = memory_size;
		break;
	case 0xf4: // exit
		if (is_write)
			exit(*value);
		break;
	}

	return 0;
}

static int misc_init(void)
{
	int err;

	err = io_table_register(&pio_table, 0xff, 1, misc_io, NULL);
	if (err < 0)
		return err;

	err = io_table_register(&pio_table, 0xf1, 1, misc_io, NULL);
	if (err < 0)
		return err;

	err = io_table_register(&pio_table, 0xf4, 1, misc_io, NULL);
	if (err < 0)
		return err;

	return io_table_register(&pio_table, 0xd1, 1, misc_io, NULL);
}

#define IRQCHIP_IO_BASE 0x2000

static int irqchip_io(void *opaque, int size, int is_write,
		      uint64_t addr, uint64_t *value)
{
	addr -= IRQCHIP_IO_BASE;

	if (is_write) {
		kvm_set_irq_level(kvm, addr, *value, NULL);
	}
	return 0;
}

#define ANNOTATION_BASEPORT 0x4000
static int annotation_handler(void *opaque, int size, int is_write,
                              uint64_t addr, uint64_t *value)
{
    // opaque is the virtual memory addr allocated to kvm.
    // value contains a pointer to the annotation buffer descriptor offset in the allocated virtual memory.
    //printf("annotation_handler: Annotations are Present in S/W\n");

    if(*value){
        systemc_annotate_function(p_kvm_cpu_adaptor[0], opaque, (opaque + *value));
    }
    else{
        printf("%s: Overflow in S/W (All Annotation Buffers Full): *value = 0x%08x\n", __func__, (uint32_t)(*value));
        exit(1);
    }
    return 0;
}

static int test_inb(void *opaque, uint16_t addr, uint8_t *value)
{
	struct io_table_entry *entry;

	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val;
		entry->handler(entry->opaque, 1, 0, addr, &val);
		*value = val;
	} else {
		*value = -1;
		printf("test_inb: inb 0x%x\n", addr);
	}

	return 0;
}

static int test_inw(void *opaque, uint16_t addr, uint16_t *value)
{
	struct io_table_entry *entry;

	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val;
		entry->handler(entry->opaque, 2, 0, addr, &val);
		*value = val;
	} else {
		*value = -1;
		printf("test_inw: inw 0x%x\n", addr);
	}

	return 0;
}

static int test_inl(void *opaque, uint16_t addr, uint32_t *value)
{
	struct io_table_entry *entry;

	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val;
		entry->handler(entry->opaque, 4, 0, addr, &val);
		*value = val;
	} else {
		*value = -1;
		printf("test_inl: inl 0x%x\n", addr);
	}

    //printf("test_inl: addr = 0x%x, value = Ox%x\n", addr, *value);
	return 0;
}

static int test_outb(void *opaque, uint16_t addr, uint8_t value)
{
	struct io_table_entry *entry;

	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val = value;
		entry->handler(entry->opaque, 1, 1, addr, &val);
	} else
        printf("test_outb: outb $0x%x, 0x%x\n", value, addr);

	return 0;
}

static int test_outw(void *opaque, uint16_t addr, uint16_t value)
{
	struct io_table_entry *entry;

	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val = value;
		entry->handler(entry->opaque, 2, 1, addr, &val);
	} else
		printf("test_outw: outw $0x%x, 0x%x\n", value, addr);

	return 0;
}

static int test_outl(void *opaque, uint16_t addr, uint32_t value)
{
	struct io_table_entry *entry;

    //printf("test_outl: addr = 0x%x, value = Ox%x\n", addr, value);
	entry = io_table_lookup(&pio_table, addr);
	if (entry) {
		uint64_t val = value;
		entry->handler(entry->opaque, 4, 1, addr, &val);
	} else
		printf("test_outl: value = $0x%x, addr = 0x%x\n", value, addr);

	return 0;
}

#ifdef KVM_CAP_SET_GUEST_DEBUG
static int test_debug(void *opaque, void *vcpu,
		      struct kvm_debug_exit_arch *arch_info)
{
	printf("test_debug\n");
	return 0;
}
#endif

static int test_halt(void *opaque, int vcpu)
{
	int n;

	sigwait(&ipi_sigmask, &n);
	kvm_inject_irq(kvm, vcpus[vcpu].id, apic_ipi_vector);
	return 0;
}

static int test_io_window(void *opaque)
{
	return 0;
}

static int test_try_push_interrupts(void *opaque)
{
	printf("------------ KVM has an interrupt-------------\n");
	return 0;
}

#ifdef KVM_CAP_USER_NMI
static void test_push_nmi(void *opaque)
{
}
#endif

static void test_post_kvm_run(void *opaque, void *vcpu)
{
}

static int test_pre_kvm_run(void *opaque, void *vcpu)
{
	return 0;
}

static int test_mmio_read(void *opaque, uint64_t addr, uint8_t *data, int len)
{
    uint64_t value;
    int i;

    if (addr < IORAM_BASE_PHYS || addr + len > IORAM_BASE_PHYS + IORAM_LEN)
    {
        fprintf(stderr, "%s: IORAM_BASE_PHYS: 0x%x, IORAM_BASE_PHYS + IORAM_LEN: 0x%x\n",
                __func__, (uint32_t) IORAM_BASE_PHYS, (uint32_t) (IORAM_BASE_PHYS + IORAM_LEN - 1));
        fprintf(stderr, "%s: Address: 0x%x, Address End: 0x%x (len = %d)\n",
                __func__, (uint32_t) addr, (uint32_t) addr+len-1, len);

        kvm_show_regs(kvm, 0);
        return 1;
    }

    value = systemc_kvm_read_memory(p_kvm_cpu_adaptor[0], addr, len, NULL, 1);
    for (i = 0; i < len; i++)
        data[i] = *((unsigned char *) &value + i);

#ifdef DEBUG
    printf("ioram read with address:0x%lx and value %x\n", (long uint32_t)addr, *data);
#endif
    return 0;
}

static int test_mmio_write(void *opaque, uint64_t addr, uint8_t *data, int len)
{
    if (addr < IORAM_BASE_PHYS || addr + len > IORAM_BASE_PHYS + IORAM_LEN)
    {
        fprintf(stderr, "%s: IORAM_BASE_PHYS: 0x%x, IORAM_BASE_PHYS + IORAM_LEN: 0x%x\n",
                __func__, (uint32_t) IORAM_BASE_PHYS, (uint32_t) (IORAM_BASE_PHYS + IORAM_LEN - 1));
        fprintf(stderr, "%s: Address: 0x%x, Address End: 0x%x (len = %d)\n",
		__func__, (uint32_t) addr, (uint32_t) addr+len-1, len);
        kvm_show_regs(kvm, 0);
        return 1;
    }

    systemc_kvm_write_memory(p_kvm_cpu_adaptor[0], addr, data, len, NULL, 1);
    return 0;
}

static int test_shutdown(void *opaque, void *env)
{
    printf("shutdown\n");
    kvm_show_regs(kvm, 0);
    exit(1);
    return 1;
}

static struct kvm_callbacks test_callbacks = {
	.inb         = test_inb,
	.inw         = test_inw,
	.inl         = test_inl,
	.outb        = test_outb,
	.outw        = test_outw,
	.outl        = test_outl,
	.mmio_read   = test_mmio_read,
	.mmio_write  = test_mmio_write,
#ifdef KVM_CAP_SET_GUEST_DEBUG
	.debug       = test_debug,
#endif
	.halt        = test_halt,
	.io_window = test_io_window,
	.try_push_interrupts = test_try_push_interrupts,
#ifdef KVM_CAP_USER_NMI
	.push_nmi = test_push_nmi,
#endif
	.post_kvm_run = test_post_kvm_run,
	.pre_kvm_run = test_pre_kvm_run,
	.shutdown = test_shutdown,
};

static void load_file(void *mem, const char *fname)
{
	int r;
	int fd;
        int bytes = 0;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	while ((r = read(fd, mem, 4096)) != -1 && r != 0)
        {
            mem += r;
            bytes += r;
        }
	if (r == -1) {
		perror("read");
		exit(1);
	}
        printf("%s: Loaded %d bytes (%d KB) in total for %s file\n", __func__, bytes, bytes/1024, fname);
}

static void enter_32(kvm_context_t kvm)
{
	struct kvm_regs regs = {
		.rsp = 0x80000,  /* 512KB */
		.rip = 0x100000, /* 1MB */
		.rflags = 2,
	};
	struct kvm_sregs sregs = {
		.cs = { 0, -1u,  8, 11, 1, 0, 1, 1, 0, 1, 0, 0 },
		.ds = { 0, -1u, 16,  3, 1, 0, 1, 1, 0, 1, 0, 0 },
		.es = { 0, -1u, 16,  3, 1, 0, 1, 1, 0, 1, 0, 0 },
		.fs = { 0, -1u, 16,  3, 1, 0, 1, 1, 0, 1, 0, 0 },
		.gs = { 0, -1u, 16,  3, 1, 0, 1, 1, 0, 1, 0, 0 },
		.ss = { 0, -1u, 16,  3, 1, 0, 1, 1, 0, 1, 0, 0 },

		.tr = { 0, 10000, 24, 11, 1, 0, 0, 0, 0, 0, 0, 0 },
		.ldt = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
		.gdt = { 0, 0 },
		.idt = { 0, 0 },
		.cr0 = 0x37,
		.cr3 = 0,
		.cr4 = 0,
		.efer = 0,
		.apic_base = 0,
		.interrupt_bitmap = { 0 },
	};

	kvm_set_regs(kvm, 0, &regs);
	kvm_set_sregs(kvm, 0, &sregs);
}

static void init_vcpu(int n)
{
	sigemptyset(&ipi_sigmask);
	sigaddset(&ipi_sigmask, IPI_SIGNAL);
	sigprocmask(SIG_UNBLOCK, &ipi_sigmask, NULL);
	sigprocmask(SIG_BLOCK, &ipi_sigmask, &kernel_sigmask);
	vcpus[n].id = n;
	vcpus[n].tid = gettid();
	vcpu = n;
	kvm_set_signal_mask(kvm, n, &kernel_sigmask);
	sem_post(&init_sem);
}

static void *do_create_vcpu(void *_n)
{
	int n = (long)_n;
	struct kvm_regs regs;

    kvm_create_vcpu(kvm, n);
    init_vcpu(n);
	sem_wait(&vcpus[n].sipi_sem);
	kvm_get_regs(kvm, n, &regs);
	regs.rip = apic_sipi_addr;
	kvm_set_regs(kvm, n, &regs);

	kvm_run(kvm, n, &vcpus[n]);
        return NULL;
}

static void start_vcpu(int n)
{
	pthread_t thread;

	sem_init(&vcpus[n].sipi_sem, 0, 0);
	pthread_create(&thread, NULL, do_create_vcpu, (void *)(long)n);
}
/*
static void usage(const char *progname)
{
	fprintf(stderr,
"Usage: %s [OPTIONS] [bootstrap] flatfile\n"
"KVM test harness.\n"
"\n"
"  -s, --smp=NUM          create a VM with NUM virtual CPUs\n"
"  -p, --protected-mode   start VM in protected mode\n"
"  -m, --memory=NUM[GMKB] allocate NUM memory for virtual machine.  A suffix\n"
"                         can be used to change the unit (default: `M')\n"
"  -h, --help             display this help screen and exit\n"
"\n"
"Report bugs to <kvm@vger.kernel.org>.\n"
		, progname);
}
*/

static void sig_ignore(int sig)
{
    write(1, "boo\n", 4);
}


int load_elf(void *vm_addr, int vm_size, char *file)
{
    Elf32_Ehdr *elf_header = NULL;  /* ELF header */
    Elf *elf = NULL;                /* Our Elf pointer for libelf */
    Elf_Scn *scn = NULL;            /* Section Descriptor */
    //GElf_Sym sym;                   /* Symbol */
    GElf_Shdr shdr;                 /* Section Header */

    int fd;                         // File Descriptor
    char *base_ptr;                 // ptr to our object in memory
    struct stat elf_stats;          // fstat struct
    int i;

    if((fd = open(file, O_RDWR)) == ERR)
    {
        printf("couldnt open %s\n", file);
        return ERR;
    }

    if((fstat(fd, &elf_stats)))
    {
        printf("could not fstat %s\n", file);
        close(fd);
        return ERR;
    }

    if(elf_stats.st_size > vm_size){
        printf("%s: ERROR ELF Binary Size Too Big; vm_size = %d, binary_size = %d\n",
               __func__, (int)vm_size, (int)elf_stats.st_size);
        return ERR;
    }

    if((base_ptr = (char *) malloc(elf_stats.st_size)) == NULL)
    {
        printf("could not malloc\n");
        close(fd);
        return ERR;
    }

    if((read(fd, base_ptr, elf_stats.st_size)) < elf_stats.st_size)
    {
        printf("could not read %s\n", file);
        free(base_ptr);
        close(fd);
        return ERR;
    }

    /* Check libelf version first */
    if(elf_version(EV_CURRENT) == EV_NONE)
    {
        printf("WARNING Elf Library is out of date!\n");
    }

    elf_header = (Elf32_Ehdr *) base_ptr;    // point elf_header at our object in memory
    elf = elf_begin(fd, ELF_C_READ, NULL);    // Initialize 'elf' pointer to our file descriptor

    printf("%s: Loading ELF Binary at vm_addr: 0x%x, Size = %d\n", __func__, (uint32_t)vm_addr, (int)elf_stats.st_size);
    printf("Section Type        Flags\tVirt Addr\tSize (bytes)\tOffset\t           Name\n");
    /* Iterate through section headers */
    while((scn = elf_nextscn(elf, scn)) != 0)
    {
        // point shdr at this section header entry
        gelf_getshdr(scn, &shdr);

        // print the Section Type
        switch(shdr.sh_type)
        {
            case SHT_NULL:              printf("SHT_NULL            ");break;
            case SHT_PROGBITS:          printf("SHT_PROGBITS        ");break;
            case SHT_SYMTAB:            printf("SHT_SYMTAB          ");break;
            case SHT_STRTAB:            printf("SHT_STRTAB          ");break;
            case SHT_RELA:              printf("SHT_RELA            ");break;
            case SHT_HASH:              printf("SHT_HASH            ");break;
            case SHT_DYNAMIC:           printf("SHT_DYNAMIC         ");break;
            case SHT_NOTE:              printf("SHT_NOTE            ");break;
            case SHT_NOBITS:            printf("SHT_NOBITS          ");break;
            case SHT_REL:               printf("SHT_REL             ");break;
            case SHT_SHLIB:             printf("SHT_SHLIB           ");break;
            case SHT_DYNSYM:            printf("SHT_DYNSYM          ");break;
            case SHT_INIT_ARRAY:        printf("SHT_INIT_ARRAY      ");break;
            case SHT_FINI_ARRAY:        printf("SHT_FINI_ARRAY      ");break;
            case SHT_PREINIT_ARRAY:     printf("SHT_PREINIT_ARRAY   ");break;
            case SHT_GROUP:             printf("SHT_GROUP           ");break;
            case SHT_SYMTAB_SHNDX:      printf("SHT_SYMTAB_SHNDX    ");break;
            case SHT_NUM:               printf("SHT_NUM             ");break;
            case SHT_LOOS:              printf("SHT_LOOS            ");break;
            case SHT_GNU_ATTRIBUTES:    printf("SHT_GNU_ATTRIBUTES  ");break;
            case SHT_GNU_HASH:          printf("SHT_GNU_HASH        ");break;
            case SHT_GNU_LIBLIST:       printf("SHT_GNU_LIBLIST     ");break;
            case SHT_CHECKSUM:          printf("SHT_CHECKSUM        ");break;
            case SHT_LOSUNW:            printf("SHT_LOSUNW          ");break;
            case SHT_SUNW_COMDAT:       printf("SHT_SUNW_COMDAT     ");break;
            case SHT_SUNW_syminfo:      printf("SHT_SUNW_syminfo    ");break;
            case SHT_GNU_verdef:        printf("SHT_GNU_verdef      ");break;
            case SHT_GNU_verneed:       printf("SHT_VERNEED         ");break;
            case SHT_GNU_versym:        printf("SHT_GNU_versym      ");break;
            case SHT_LOPROC:            printf("SHT_LOPROC          ");break;
            case SHT_HIPROC:            printf("SHT_HIPROC          ");break;
            case SHT_LOUSER:            printf("SHT_LOUSER          ");break;
            case SHT_HIUSER:            printf("SHT_HIUSER          ");break;
            default:                    printf("(none)              ");break;
        }

        // print the section header Flags
        if(shdr.sh_flags & SHF_WRITE) { printf("W"); }
        if(shdr.sh_flags & SHF_ALLOC) { printf("A"); }
        if(shdr.sh_flags & SHF_EXECINSTR) { printf("X"); }
        if(shdr.sh_flags & SHF_STRINGS) { printf("S"); }
        printf("\t\t");

        // Virt Addr
        printf("0x%08llx\t", (uint64_t)shdr.sh_addr);
        // Size (bytes)
        printf("%lld\t\t", (uint64_t)shdr.sh_size);
        // Offset
        printf("0x%llx\t", (uint64_t)shdr.sh_offset);

        // the shdr Name is in a string table, libelf uses elf_strptr() to find it
        // using the e_shstrndx value from the elf_header
        printf("%15s\t", elf_strptr(elf, elf_header->e_shstrndx, shdr.sh_name));

        // Load binary to memory address.
        for(i = 0; strcmp(section_copy[i], "") != 0; i++)
        {
            if(strcmp(section_copy[i], elf_strptr(elf, elf_header->e_shstrndx, shdr.sh_name)) == 0)
            {
                int n = 0;
                Elf_Data *edata = NULL;         /* Data Descriptor */
                while( (n < shdr.sh_size) && ((edata = elf_getdata(scn, edata)) != NULL))
                {
                    memcpy(vm_addr + shdr.sh_addr + n, edata->d_buf, edata->d_size);
                    n += edata->d_size;
                }
                printf("Loaded ...  %7d Bytes @ 0x%x (KVM: 0x%x)", n,
                       (uint32_t)(vm_addr + shdr.sh_addr), (uint32_t) shdr.sh_addr);
            }
        }

        if(strcmp(section_bss, elf_strptr(elf, elf_header->e_shstrndx, shdr.sh_name)) == 0)
        {
            memset(vm_addr + shdr.sh_addr, 0, shdr.sh_size);
            printf("Initialized %7d Bytes @ 0x%x (KVM: 0x%x)", (int)shdr.sh_size,
                   (uint32_t)(vm_addr + shdr.sh_addr), (uint32_t)(shdr.sh_addr));
        }

        printf("\n");
    }

    free(base_ptr);
    close(fd);
    return 0;
}

int soc_kvm_init(char *bootstrap, char *elf_file)
{
	unsigned char *vm_mem;
    uint32_t phys_start = 0x0;
	int i;
	bool enter_protected_mode = false;

	signal(IPI_SIGNAL, sig_ignore);

	vcpus = calloc(ncpus, sizeof *vcpus);
	if (!vcpus) {
		fprintf(stderr, "calloc failed\n");
		return 1;
	}

	kvm = kvm_init(&test_callbacks, 0);
	if (!kvm) {
		fprintf(stderr, "kvm_init failed\n");
		return 1;
	}

    if (kvm_create(kvm, memory_size, (void **)&vm_mem) < 0) {
        kvm_finalize(kvm);
        fprintf(stderr, "kvm_create failed\n");
        return 1;
    }

    printf("%s: Creating KVM Physical Memory ... From: 0x%x \tTo: 0x%x\n", __func__,
            (uint32_t) phys_start, (uint32_t) (phys_start + memory_size));

    vm_mem = kvm_create_phys_mem(kvm, phys_start, memory_size /* len */, 0 /* log */, 1 /* writable */);
    if(vm_mem == NULL)
    {
        kvm_finalize(kvm);
        fprintf(stderr, "kvm_create_phys_mem failed\n");
        return 2;
    }

	if (enter_protected_mode){
		enter_32(kvm);
    }else{
        printf("%s: Loading %s ... at 0x%x (KVM PHY: 0x%x)\n", __func__, bootstrap, (uint32_t) vm_mem + 0xf0000, 0xf0000);
        load_file(vm_mem + 0xf0000, bootstrap);
    }

    load_elf(vm_mem, memory_size, elf_file);

	apic_init();
	misc_init();

	io_table_register(&pio_table, IRQCHIP_IO_BASE, 0x20, irqchip_io, NULL);

    // Register the Annotation Handler; Also pass the Virtual Memory Address allocated to KVM
    io_table_register(&pio_table, ANNOTATION_BASEPORT, 0x04, annotation_handler, vm_mem);

    // Register the HostTime Handler
    io_table_register(&pio_table, HOSTTIME_BASEPORT, 0x4, hosttime_handler, &hosttime_instance);

	sem_init(&init_sem, 0, 0);

    for (i = 0; i < ncpus; ++i)
        start_vcpu(i);

    for (i = 0; i < ncpus; ++i)
        sem_wait(&init_sem);

	soc_kvm_init_data.ncpus = ncpus;
	soc_kvm_init_data.memory_size = memory_size;
	soc_kvm_init_data.vm_mem = vm_mem;

	return 0;
}

void soc_kvm_run()
{
    int ret = 0;

    if(init_hosttime(&hosttime_instance, "hosttime_kvm.txt"))
    {
        printf("%s: Error Initializing Hosttime Instance\n", __func__);
        return;
    }

    io_table_print(&pio_table);

    printf("%s: Calling kvm_run ... \n", __func__);
    ret = kvm_run(kvm, 0, &vcpus[0]);

    printf("%s Exited; Return Value = %d\n", __func__, ret);
}

int soc_load_target_section(char * target_binary, char * section_name)
{
    uint8_t  * kvm_mem_base_addr = (uint8_t *) soc_kvm_init_data.vm_mem;
    uint32_t   binary_load_addr  = 0x0;         // Where this section should be loaded ?
    uint32_t   binary_size       = 0;
    uint32_t   binary_value      = 0;
    uint32_t   bytes_loaded      = 0;

    char section_file_path[256] = {0};
    strncpy(section_file_path, target_binary, 256);
    strcat(section_file_path, section_name);

    fprintf(stdout, "Loading %s ... ", section_file_path);

    FILE * target_sect_file = fopen(section_file_path, "r");
    if(! target_sect_file)
    {
        fprintf(stderr, "\nError! Opening the Target Section File: %s\n", target_binary);
        return (-1);
    }

    if(fread((void *) & binary_load_addr, sizeof(uint32_t), 1, target_sect_file) != 1)
    {
        fprintf(stderr, "\nError! Reading from target section file\n");
        return (-1);
    }

    if(fread((void *) & binary_size, sizeof(uint32_t), 1, target_sect_file) != 1)
    {
        fprintf(stderr, "\nError! Reading from target section file\n");
        return (-1);
    }

    fprintf(stdout, " at 0x%08x [0x%x bytes]\n", binary_load_addr, binary_size);

    while(bytes_loaded < binary_size)
    {
        if(fread((void *) & binary_value, sizeof(uint32_t), 1, target_sect_file) != 1)
        {
            fprintf(stderr, "Error Reading from target section file\n");
            return (-1);
        }

        // Write to KVM Memory
        *((uint32_t *)(kvm_mem_base_addr + binary_load_addr)) = binary_value;

#if 0
        if(strcmp(section_name, ".cinit") == 0)
        {
            fprintf(stdout, "[0x%08x] = 0x%08x\n", (uint32_t *)(binary_load_addr),
                    *(uint32_t *)(kvm_mem_base_addr + binary_load_addr));
        }

        if(strcmp(section_name, ".far") == 0)
        {
            fprintf(stdout, "[%x] = %x\n", (uint32_t *)(binary_load_addr),
                    *(uint32_t *)(kvm_mem_base_addr + binary_load_addr));
        }
#endif

        binary_load_addr += sizeof(uint32_t);
        bytes_loaded += sizeof(uint32_t);
    }

    if(target_sect_file)
    {
        fclose(target_sect_file);
        target_sect_file = NULL;
    }

    return (0);
}

// The target binary path; specified in platform's main.cpp (tuzki)
extern char *p_target_binary;

int soc_load_target_binary()
{
    char * sections_to_load[] = {".text", ".data", ".cinit", ".const", ""};
    uint32_t i = 0;

    if(p_target_binary)
    {
        for(i = 0; strcmp(sections_to_load[i], "") != 0; i++)
        {
            if(soc_load_target_section(p_target_binary, sections_to_load[i]))
            {
                fprintf(stderr, "Error Loading Target .text Section\n");
                return (-1);
            }
        }
    }
    return (0);
}

int soc_fill_memory_pattern(uint32_t size, uint32_t pattern)
{
    uint32_t *  address = (uint32_t *) soc_kvm_init_data.vm_mem;
    uint32_t *  kvm_phy_addr = 0x0;

    uint32_t * curr_ptr = address;
    uint32_t * end_ptr  = address + ( size / sizeof(uint32_t) );

    while (curr_ptr < end_ptr)
    {
        if(pattern)
            *curr_ptr = pattern;
        else
            *curr_ptr = (uint32_t) kvm_phy_addr + 0x56341200;

        curr_ptr++;
        kvm_phy_addr++;
    }

    printf("%s: Filled Memory from 0x%08x to 0x%08x (KVM PHY:0x%08x - 0x%08x)\n",
            __func__, (uint32_t) address,
            (uint32_t) ((unsigned char *) end_ptr - 1),
            (uint32_t) ((unsigned char *) address - soc_kvm_init_data.vm_mem),
            (uint32_t) ((unsigned char *) end_ptr - soc_kvm_init_data.vm_mem - 1));

    return (0);
}

void soc_memory_dump(unsigned long addr, unsigned long size)
{
    unsigned char *p;
    unsigned long n;

    size &= ~15; /* mod 16 */
    if (!size) return;

    p = (unsigned char *) soc_kvm_init_data.vm_mem + addr;

    for (n = 0; n < size; n += 16) {
        printf("  0x%08lx: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
               addr + n, p[n + 0], p[n + 1], p[n + 2], p[n + 3], p[n + 4], p[n + 5], p[n + 6], p[n + 7],
                         p[n + 8], p[n + 9], p[n + 10], p[n + 11], p[n + 12], p[n + 13], p[n + 14], p[n + 15]);
    }

    return;
}

int soc_erase_memory(int size)
{
    uint32_t *  address = (uint32_t *) soc_kvm_init_data.vm_mem;

    uint32_t * curr_ptr = address;
    uint32_t * end_ptr  = address + ( size / sizeof(uint32_t) );

    while (curr_ptr < end_ptr)
    {
        *curr_ptr = 0x0;
        curr_ptr++;
    }

    printf("%s: Erased Memory from 0x%08x to 0x%08x (KVM PHY:0x%08x - 0x%08x)\n",
            __func__, (uint32_t) address,
            (uint32_t) ((unsigned char *) end_ptr - 1),
            (uint32_t) ((unsigned char *) address - soc_kvm_init_data.vm_mem),
            (uint32_t) ((unsigned char *) end_ptr - soc_kvm_init_data.vm_mem - 1));

    // Verify that all memory contents are actually zero?
    curr_ptr = address;
    while(curr_ptr < end_ptr)
    {
        if(*curr_ptr != 0x0)
        {
            printf("%s: Non Zero Memory Contents Found: 0x%08x = 0x%08x (KVM PHY:0x%08x = 0x%08x)\n",
                    __func__, (uint32_t) curr_ptr,
                              (uint32_t) (*curr_ptr),
                              (uint32_t) ((unsigned char *) curr_ptr - (unsigned char *) address),
                              (uint32_t) (*curr_ptr));
            return (-1);
        }
        curr_ptr++;
    }

    //kvm_show_regs(kvm, 0);
    //kvm_dump_vcpu(kvm, 0);
    return (0);
}

int soc_verify_memory(int size)
{
    uint32_t * address  = (uint32_t *) soc_kvm_init_data.vm_mem;

    uint32_t * curr_ptr = address;
    uint32_t * end_ptr  = address + ( size / sizeof(uint32_t) );

    kvm_show_regs(kvm, 0);
    //kvm_dump_vcpu(kvm, 0);
    printf("%s: Verifying Memory\n", __func__);

    while (curr_ptr < end_ptr)
    {
        if(*curr_ptr != 0x0)
        {
            printf("[%08X] = 0x%08X\n", (unsigned char *) curr_ptr - (unsigned char *) address, *curr_ptr);
            //printf("%s: Memory Test Failed\n", __func__);
            //return (-1);
        }
        curr_ptr++;
    }

    //printf("%s: Memory Test Passed: %05d\r", __func__, test_count++);
    fflush(stdout);
    return (0);
}
