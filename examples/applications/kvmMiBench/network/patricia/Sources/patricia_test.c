/*
 * patricia_test.c
 *
 * Patricia trie test code.
 *
 * This code is an example of how to use the Patricia trie library for
 * doing longest-prefix matching.  We begin by adding a default
 * route/default node as the head of the Patricia trie.  This will become
 * an initialization functin (pat_init) in the future.  We then read in a
 * set of IPv4 addresses and network masks from "pat_test.txt" and insert
 * them into the Patricia trie.  I haven't _yet_ added example of searching
 * and removing nodes.
 *
 * Compiling the library:
 *     gcc -g -Wall -c patricia.c
 *     ar -r libpatricia.a patricia.o
 *     ranlib libpatricia.a
 *
 * Compiling the test code (or any other file using libpatricia):
 *     gcc -g -Wall -I. -L. -o ptest patricia_test.c -lpatricia
 *
 * Matthew Smart <mcsmart@eecs.umich.edu>
 *
 * Copyright (c) 2000
 * The Regents of the University of Michigan
 * All rights reserved
 *
 * $Id: patricia_test.c,v 1.1.1.1 2000/11/06 19:53:17 mguthaus Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

//#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

//#include <rpc/rpc.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>

#include "patricia.h"
#include <Processor/Profile.h>
#include "custom_mem.h"

#ifdef USE_CUSTOM_MEM
custom_memory_t custom_mem;
#endif

# define htonl(x)	(x)

/* Internet address.  */
typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };


struct MyNode {
	int foo;
	double bar;
};

#ifdef  MEASURE_ACCURACY
    /* Copied the following definitions from qemu_wrapper_cts.h & kvm_cpu_wrapper.h */
    #define QEMU_ADDR_BASE                              0x82000000
    #define KVM_ADDR_BASE                               0xE0000000
    #define LOG_DELTA_STATS                             0x0058
#ifdef PLATFORM_QEMU
    #define ADDR_BASE                                   QEMU_ADDR_BASE
#else
    #define ADDR_BASE                                   KVM_ADDR_BASE
#endif
#endif

int main(int argc, char *argv[]) 
{
#ifdef MEASURE_ACCURACY
    volatile int *LOG_ADDR = ADDR_BASE + LOG_DELTA_STATS;
    *LOG_ADDR = 1;
    real_main(argc, argv, 0);
    *LOG_ADDR = 0;     /* Writing Zero to this Address will cause QEMU/KVM to exit */
#elif DISABLE_APP_REPEAT /* This option is useful in case we want to execute once or We want to use Analyzer */
    real_main(argc, argv, 0);
#else
    int app_repeat_count;
    for (app_repeat_count = 0; app_repeat_count < 5; app_repeat_count++)
        real_main(argc, argv, app_repeat_count);

    CPU_PROFILE_FLUSH_DATA();
#endif
    return 0;
}

int real_main(int argc, char **argv, int app_repeat_count)
{
#ifdef USE_CUSTOM_MEM
    init_mem(& custom_mem);
#endif
	struct ptree *phead;
	struct ptree *p,*pfind;
	struct ptree_mask *pm;
	FILE *fp;
	char line[128];
	char addr_str[16];
	struct in_addr addr;
	unsigned long mask=0xffffffff;
	float time;

    int myargc;
    char *myargv[2];

    FILE *fout;

    printf("PATRICIA: In main function : For %d time\n\n", app_repeat_count);

    myargc = 2;
    myargv[0] = "./patricia";
    myargv[1] = "/devices/disk/simulator/0";

	if (myargc<2) {
		printf("Usage: %s <TCP stream>\n", argv[0]);
		exit(-1);
	}

	/*
	 * Open file of IP addresses and masks.
	 * Each line looks like:
	 *    10.0.3.4 0xffff0000
	 */
	if ((fp = fopen(myargv[1], "r")) == NULL) {
		printf("File %s doesn't seem to exist\n",myargv[1]);
		exit(0);
	}

	if ((fout = fopen("/devices/disk/simulator/2", "w")) == NULL) {
		printf("File /devices/disk/simulator/2 doesn't seem to exist\n");
		exit(0);
	}

	/*
	 * Initialize the Patricia trie by doing the following:
	 *   1. Assign the head pointer a default route/default node
	 *   2. Give it an address of 0.0.0.0 and a mask of 0x00000000
	 *      (matches everything)
	 *   3. Set the bit position (p_b) to 0.
	 *   4. Set the number of masks to 1 (the default one).
	 *   5. Point the head's 'left' and 'right' pointers to itself.
	 * NOTE: This should go into an intialization function.
	 */
#ifdef USE_CUSTOM_MEM
	phead = (struct ptree *) alloc_mem(& custom_mem);
#else
	phead = (struct ptree *)malloc(sizeof(struct ptree));
#endif
	if (!phead) {
		perror("Allocating p-trie node");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
		exit(0);
	}
	bzero(phead, sizeof(*phead));

#ifdef USE_CUSTOM_MEM
	phead->p_m = (struct ptree_mask *)alloc_mem(& custom_mem);
#else
	phead->p_m = (struct ptree_mask *)malloc(sizeof(struct ptree_mask));
#endif
	if (!phead->p_m) {
		perror("Allocating p-trie mask data");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
		exit(0);
	}
	bzero(phead->p_m, sizeof(*phead->p_m));
	pm = phead->p_m;
#ifdef USE_CUSTOM_MEM
	pm->pm_data = (struct MyNode *)alloc_mem(& custom_mem);
#else
	pm->pm_data = (struct MyNode *)malloc(sizeof(struct MyNode));
#endif
	if (!pm->pm_data) {
		perror("Allocating p-trie mask's node data");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
		exit(0);
	}
	bzero(pm->pm_data, sizeof(*pm->pm_data));
	/*******
	 *
	 * Fill in default route/default node data here.
	 *
	 *******/
	phead->p_mlen = 1;
	phead->p_left = phead->p_right = phead;

	/*
	 * The main loop to insert nodes.
	 */
	while (fgets(line, 128, fp)) {
        CPU_PROFILE_IO_START();
		/*
		 * Read in each IP address and mask and convert them to
		 * more usable formats.
		 */
		sscanf(line, "%f %d", &time, (unsigned int *)&addr);
		//inet_aton(addr_str, &addr);
        CPU_PROFILE_IO_END();

        CPU_PROFILE_COMP_START();
		/*
		 * Create a Patricia trie node to insert.
		 */
#ifdef USE_CUSTOM_MEM
	    p = (struct ptree *)alloc_mem(& custom_mem);
#else
		p = (struct ptree *)malloc(sizeof(struct ptree));
#endif
		if (!p) {
			perror("Allocating p-trie node");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
			exit(0);
		}
		bzero(p, sizeof(*p));

		/*
		 * Allocate the mask data.
		 */
#ifdef USE_CUSTOM_MEM
	    p->p_m = (struct ptree_mask *)alloc_mem(& custom_mem);
#else
		p->p_m = (struct ptree_mask *)malloc(sizeof(struct ptree_mask));
#endif
		if (!p->p_m) {
			perror("Allocating p-trie mask data");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
			exit(0);
		}
		bzero(p->p_m, sizeof(*p->p_m));

		/*
		 * Allocate the data for this node.
		 * Replace 'struct MyNode' with whatever you'd like.
		 */
		pm = p->p_m;
#ifdef USE_CUSTOM_MEM
	    pm->pm_data = (struct MyNode *)alloc_mem(& custom_mem);
#else
		pm->pm_data = (struct MyNode *)malloc(sizeof(struct MyNode));
#endif
		if (!pm->pm_data) {
			perror("Allocating p-trie mask's node data");
#ifdef USE_CUSTOM_MEM
        print_mem_state(& custom_mem);
#endif
			exit(0);
		}
		bzero(pm->pm_data, sizeof(*pm->pm_data));

		/*
		 * Assign a value to the IP address and mask field for this
		 * node.
		 */
		p->p_key = addr.s_addr;		/* Network-byte order */
		p->p_m->pm_mask = htonl(mask);

		pfind=pat_search(addr.s_addr,phead);
		//printf("%08x %08x %08x\n",p->p_key, addr.s_addr, p->p_m->pm_mask);
		//if(pfind->p_key==(addr.s_addr&pfind->p_m->pm_mask))
        CPU_PROFILE_COMP_END();

		if(pfind->p_key==addr.s_addr)
		{
            CPU_PROFILE_IO_START();
			//fprintf(fout, "%f %08x: ", time, addr.s_addr);
            fprintf(fout, "%d %08x: ", (int)time, addr.s_addr);
			fprintf(fout, "Found.\n");
            CPU_PROFILE_IO_END();
		}
		else
		{
            /*
            * Insert the node.
            * Returns the node it inserted on success, 0 on failure.
            */
            //printf("%08x: ", addr.s_addr);
            //printf("Inserted.\n");
            CPU_PROFILE_COMP_START();
			p = pat_insert(p, phead);
            CPU_PROFILE_COMP_END();
		}
		if (!p) {
			fprintf(stderr, "Failed on pat_insert\n");
			exit(0);
		}
	}

	fflush(fout);
	fclose(fp);
	fclose(fout);

    printf("\nDone\n");
    return 0;
}
