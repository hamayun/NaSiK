#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

int main(void)
{
	char buffer[1024]; 
	int bytecount; 

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

	bytecount = 0; 
	while (1)
	{
		fread(&buffer[bytecount], 1, 1, fpr); 
		//buffer[bytecount] = fgetc(fpr);

		printf("%c", buffer[bytecount]); 

		fwrite(&buffer[bytecount], 1, 1, fpw); 
		//fputc(buffer[bytecount], fpw);

		bytecount++; 

		if(bytecount == 60) break; 
	}	

	fclose(fpr); 
	fclose(fpw); 

	printf ("BLK_DEV_TEST: Exiting Main Function\r\n");
	return(0); 
}

