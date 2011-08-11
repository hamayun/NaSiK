#include <stdio.h>
#include <Processor/Profile.h>

#include "custom_mem.h"

#ifdef USE_CUSTOM_MEM
custom_memory_t custom_mem;
#endif

#define NUM_NODES                          100
#define NONE                               9999

FILE * fout;

struct _NODE
{
  int iDist;
  int iPrev;
};
typedef struct _NODE NODE;

QITEM *qHead = NULL;

int AdjMatrix[NUM_NODES][NUM_NODES];

int g_qCount = 0;
NODE rgnNodes[NUM_NODES];
int ch;
int iPrev, iNode;
int i, iCost, iDist;


void print_path (NODE *rgnNodes, int chNode)
{
  if (rgnNodes[chNode].iPrev != NONE)
    {
      print_path(rgnNodes, rgnNodes[chNode].iPrev);
    }
  fprintf (fout, " %d", chNode);
  //fflush(stdout);
  fflush(fout);
}


void enqueue (int iNode, int iDist, int iPrev)
{
#ifdef USE_CUSTOM_MEM
  QITEM *qNew = alloc_mem(& custom_mem);
#else
  QITEM *qNew = (QITEM *) malloc(sizeof(QITEM));
#endif
  QITEM *qLast = qHead;

  if (!qNew)
    {
      fprintf(stderr, "Out of memory.\n");
#ifdef USE_CUSTOM_MEM
      print_mem_state(& custom_mem);
#endif
      exit(1);
    }
  qNew->iNode = iNode;
  qNew->iDist = iDist;
  qNew->iPrev = iPrev;
  qNew->qNext = NULL;

  if (!qLast)
    {
      qHead = qNew;
    }
  else
    {
      while (qLast->qNext) qLast = qLast->qNext;
      qLast->qNext = qNew;
    }
  g_qCount++;
  //               ASSERT(g_qCount);
}


void dequeue (int *piNode, int *piDist, int *piPrev)
{
  QITEM *qKill = qHead;

  if (qHead)
    {
      //                 ASSERT(g_qCount);
      *piNode = qHead->iNode;
      *piDist = qHead->iDist;
      *piPrev = qHead->iPrev;
      qHead = qHead->qNext;
#ifdef USE_CUSTOM_MEM
      free_mem (& custom_mem, qKill);
#else
      free(qKill);
#endif
      g_qCount--;
    }
}


int qcount (void)
{
  return(g_qCount);
}

int dijkstra(int chStart, int chEnd)
{
  for (ch = 0; ch < NUM_NODES; ch++)
    {
      rgnNodes[ch].iDist = NONE;
      rgnNodes[ch].iPrev = NONE;
    }

  if (chStart == chEnd)
    {
      CPU_PROFILE_IO_START();
      fprintf(fout, "Shortest path is 0 in cost. Just stay where you are.\n");
      CPU_PROFILE_IO_END();
    }
  else
    {
      CPU_PROFILE_COMP_START();
      rgnNodes[chStart].iDist = 0;
      rgnNodes[chStart].iPrev = NONE;

      enqueue (chStart, 0, NONE);

     while (qcount() > 0)
	{
	  dequeue (&iNode, &iDist, &iPrev);
	  for (i = 0; i < NUM_NODES; i++)
	    {
	      if ((iCost = AdjMatrix[iNode][i]) != NONE)
		{
		  if ((NONE == rgnNodes[i].iDist) ||
		      (rgnNodes[i].iDist > (iCost + iDist)))
		    {
		      rgnNodes[i].iDist = iDist + iCost;
		      rgnNodes[i].iPrev = iNode;
		      enqueue (i, iDist + iCost, iNode);
		    }
		}
	    }
	}
      CPU_PROFILE_COMP_END();

      CPU_PROFILE_IO_START();
      fprintf(fout, "Shortest path is %d in cost. ", rgnNodes[chEnd].iDist);
      fprintf(fout, "Path is: ");
      print_path(rgnNodes, chEnd);
      fprintf(fout, "\n");
      CPU_PROFILE_IO_END();
    }
}

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
    for (app_repeat_count = 0; app_repeat_count < 10; app_repeat_count++)
        real_main(argc, argv, app_repeat_count);

    CPU_PROFILE_FLUSH_DATA();
#endif
    return 0;
}

int real_main(int argc, char *argv[], int app_repeat_count) 
{
#ifdef USE_CUSTOM_MEM
    init_mem(& custom_mem);
#endif
  int i,j,k;
  FILE *fp;

//  if (argc<2) {
//    fprintf(stderr, "Usage: dijkstra <filename>\n");
//    fprintf(stderr, "Only supports matrix size is #define'd.\n");
//  }

  /* open the adjacency matrix file */
//  fp = fopen (argv[1],"r");
    printf("DIJKSTRA: In main function : For %d time\n\n", app_repeat_count);

  fp = fopen ("/devices/disk/simulator/0","r");
  fout = fopen ("/devices/disk/simulator/2","w");

  CPU_PROFILE_IO_START();
  /* make a fully connected matrix */
  for (i=0;i<NUM_NODES;i++) {
    for (j=0;j<NUM_NODES;j++) {
        /* make it more sparce */
        fscanf(fp,"%d",&k);
        AdjMatrix[i][j]= k;
    }
  }
  CPU_PROFILE_IO_END();

  /* finds 10 shortest paths between nodes */
  for (i=0,j=NUM_NODES/2;i<100;i++,j++) {
    j=j%NUM_NODES;
    dijkstra(i,j);
  }

  fclose(fp);
  fclose(fout);
  printf("\nDone\n");
  return (0);
}

