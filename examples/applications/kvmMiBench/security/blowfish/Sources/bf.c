#include <stdio.h>
#include "blowfish.h"

int
main(int argc, char *argv[])
{
	BF_KEY key;
	unsigned char ukey[8];
	unsigned char indata[40],outdata[40],ivec[8];
	int num;
	int by=0,i=0;
	int encordec=-1;
	char *cp,ch;
	FILE *fp,*fp2;

    int myargc;
    char *myargv[4];

    /* Dump Host Time to File
    */
    int checkpoint = 0;
    volatile int *htime;
    htime = (int *)0xCE000000;  
    *htime = checkpoint++;
  
	/* ./bf e input_large.asc output_large.enc 1234567890abcdeffedcba0987654321 */
	myargc = 5;
	myargv[0] = "./bf";
	myargv[1] = "e";
	myargv[2] = "/devices/disk/simulator/0";
	myargv[3] = "/devices/disk/simulator/2";
	myargv[4] = "1234567890abcdeffedcba0987654321";




if (myargc<3)
{
	printf("Usage: blowfish {e|d} <intput> <output> key\n");
	exit(-1);
}

if (*myargv[1]=='e' || *myargv[1]=='E')
	encordec = 1;
else if (*myargv[1]=='d' || *myargv[1]=='D')
	encordec = 0;
else
{
	printf("Usage: blowfish {e|d} <intput> <output> key\n");
	exit(-1);
}
					

/* Read the key */
cp = myargv[4];
while(i < 64 && *cp)    /* the maximum key length is 32 bytes and   */
{                       /* hence at most 64 hexadecimal digits      */
	ch = toupper(*cp++);            /* process a hexadecimal digit  */
	if(ch >= '0' && ch <= '9')
		by = (by << 4) + ch - '0';
	else if(ch >= 'A' && ch <= 'F')
		by = (by << 4) + ch - 'A' + 10;
	else                            /* error if not hexadecimal     */
	{
		printf("key must be in hexadecimal notation\n");
		exit(-1);
	}

	/* store a key byte for each pair of hexadecimal digits         */
	if(i++ & 1)
		ukey[i / 2 - 1] = by & 0xff;
}

BF_set_key(&key,8,ukey);

if(*cp)
{
	printf("Bad key value.\n");
	exit(-1);
}

/* open the input and output files */
if ((fp = fopen(myargv[2],"r"))==0)
{
	printf("Usage: blowfish {e|d} <intput> <output> key\n");
	exit(-1);
};
if ((fp2 = fopen(myargv[3],"w"))==0)
{
	printf("Usage: blowfish {e|d} <intput> <output> key\n");
	exit(-1);
};

i=0;
while(!feof(fp))
{
	int j;
	while(!feof(fp) && i<40)
		indata[i++]=getc(fp);

	BF_cfb64_encrypt(indata,outdata,i,&key,ivec,&num,encordec);

	for(j=0;j<i;j++)
	{
		/*printf("%c",outdata[j]);*/
		fputc(outdata[j],fp2);
	}
	i=0;
}

close(fp);
close(fp2);

*htime = checkpoint++;

exit(1);
}



