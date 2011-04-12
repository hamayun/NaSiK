#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

#define BUF_SIZE 64

int main(void)
{
	char buffer[BUF_SIZE]; 
	int bytesread; 
	int byteswritten; 
	int tr = 0, tw = 0; 

	printf ("BLK_DEV_TEST: Entered Main Function\r\n");

	FILE *fpr = fopen("/devices/disk/simulator/0", "r");
	if(fpr == NULL){
		printf ("BLK_DEV_TEST: File Open Error (For Reading)\r\n");
		return(-1); 	
	}

	printf ("BLK_DEV_TEST: Going to Open the Second File\r\n");

	FILE *fpw = fopen("/devices/disk/simulator/1", "w");
	if(fpw == NULL){
		printf ("BLK_DEV_TEST: File Open Error (For Writing)\r\n");
		return(-1); 	
	}

	printf("BLK_DEV_TEST: Copying .... \n");

	while (!feof(fpr))
	{
		bytesread = fread(buffer, 1, BUF_SIZE, fpr);
		tr += bytesread; 
		byteswritten = fwrite(buffer, 1, bytesread, fpw); 
		tw += byteswritten; 

		printf("bytesread = %d, byteswritten = %d, tr = %d, tw = %d\n", bytesread, byteswritten, tr, tw);  
	}	

	fclose(fpr); 
	fclose(fpw); 

	printf ("BLK_DEV_TEST: Exiting Main Function\r\n");
	return(0); 
}

