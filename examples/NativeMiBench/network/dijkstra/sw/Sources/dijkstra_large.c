#include <stdio.h>

#define NUM_NODES                          100
#define NONE                               9999

FILE * fout;

struct _NODE
{
  int iDist;
  int iPrev;
};
typedef struct _NODE NODE;

struct _QITEM
{
  int iNode;
  int iDist;
  int iPrev;
  struct _QITEM *qNext;
};
typedef struct _QITEM QITEM;

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
  QITEM *qNew = (QITEM *) malloc(sizeof(QITEM));
  QITEM *qLast = qHead;
  
  if (!qNew) 
    {
      fprintf(stderr, "Out of memory.\n");
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
      free(qKill);
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
      fprintf(fout, "Shortest path is 0 in cost. Just stay where you are.\n");
    }
  else
    {
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
      
      fprintf(fout, "Shortest path is %d in cost. ", rgnNodes[chEnd].iDist);
      fprintf(fout, "Path is: ");
      print_path(rgnNodes, chEnd);
      fprintf(fout, "\n");
    }
}

/*
 * Send command to the hosttime component to
 * Dump System Time at this particuler instant i.e. checkpoint.
 */
#include <Processor/IO.h>
extern unsigned int PLATFORM_HOSTTIME_BASE;
void DumpHostTime(int checkpoint)
{
    if(PLATFORM_HOSTTIME_BASE){
        cpu_write (UINT32, PLATFORM_HOSTTIME_BASE, checkpoint);
    }
    else
    {
        printf(stderr, "Invalid Address PLATFORM_HOSTTIME_BASE\n");
        exit(1); 
    }
}

int main(int argc, char *argv[]) {
  int i,j,k;
  FILE *fp;
  
//  if (argc<2) {
//    fprintf(stderr, "Usage: dijkstra <filename>\n");
//    fprintf(stderr, "Only supports matrix size is #define'd.\n");
//  }

  /* open the adjacency matrix file */
//  fp = fopen (argv[1],"r");

  /* Dump Host Time to File
   */
  int checkpoint = 1;
  DumpHostTime(checkpoint++);

  fp = fopen ("/devices/disk/simulator/0","r");
  fout = fopen ("/devices/disk/simulator/1","w");

  /* make a fully connected matrix */
  for (i=0;i<NUM_NODES;i++) {
    for (j=0;j<NUM_NODES;j++) {
      /* make it more sparce */
      fscanf(fp,"%d",&k);
			AdjMatrix[i][j]= k;
    }
  }


  /* finds 10 shortest paths between nodes */
  for (i=0,j=NUM_NODES/2;i<100;i++,j++) {
			j=j%NUM_NODES;
      dijkstra(i,j);
  }
  fclose(fp);
  fclose(fout);

  DumpHostTime(checkpoint++);

  exit(0);
}