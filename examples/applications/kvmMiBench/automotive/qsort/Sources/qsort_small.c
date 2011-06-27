#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define UNLIMIT
#define MAXARRAY 60000 /* this number, if too large, will cause a seg. fault!! */

struct myStringStruct {
  char qstring[128];
};

int compare(const void *elem1, const void *elem2)
{
  int result;
  
  result = strcmp((*((struct myStringStruct *)elem1)).qstring, (*((struct myStringStruct *)elem2)).qstring);

  return (result < 0) ? 1 : ((result == 0) ? 0 : -1);
}

struct myStringStruct array[MAXARRAY];

int
main(int argc, char *argv[]) {
  FILE *fp;
  FILE *fp1;
  FILE *fp2;
  int i,count=0;
  
#if 0
  if (argc<2) {
    fprintf(stderr,"Usage: qsort_small <file>\n");
    exit(-1);
  }
  else {
    fp = fopen(argv[1],"r");
#endif

    fp  = fopen("/devices/disk/simulator/0", "r");
	fp1 = fopen("/devices/disk/simulator/1", "r");
	fp2 = fopen("/devices/disk/simulator/2", "w");
    
    while((fscanf(fp, "%s", &array[count].qstring) == 1) && (count < MAXARRAY)) {
	 count++;
    }
#if 0
  }
#endif
  printf("\nSorting %d elements.\n\n",count);

  /* Dump Host Time to File
   */
  int checkpoint = 0;
  volatile int *htime;
  htime = (int *)0xCE000000;  
  *htime = checkpoint++;

  qsort(array,count,sizeof(struct myStringStruct),compare);
  
  *htime = checkpoint++;

  for(i=0;i<count;i++)
    fprintf(fp2, "%s\n", array[i].qstring);

  fclose(fp);
  fclose(fp1);
  fclose(fp2);

  *htime = checkpoint++;

  return 0;
}
