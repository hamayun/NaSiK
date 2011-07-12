#include <stdio.h>
#include "blowfish.h"
#include <Processor/Profile.h>

int
main(int argc, char *argv[])
{
    int app_repeat_count;
    for (app_repeat_count = 0; app_repeat_count < 5; app_repeat_count++)
    {
    BF_KEY key = {0,0};
    unsigned char ukey[32] = {0};
    unsigned char indata[40] = {0}, outdata[40] = {0}, ivec[8] = {0};
    int num;
    int by=0,i=0;
    int encordec=-1;
    char *cp,ch;
    FILE *fp,*fp2;

    int myargc;
    char *myargv[5];

    /* ./bf e input_large.asc output_large.enc 1234567890abcdeffedcba0987654321 */
    myargc = 5;
    myargv[0] = "./bf";
    myargv[1] = "e";
    myargv[2] = "/devices/disk/simulator/0";
    myargv[3] = "/devices/disk/simulator/2";
    myargv[4] = "1234567890abcdeffedcba0987654321";

    printf("BLOWFISH: In main function : For %d time\n\n", app_repeat_count);
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

    CPU_PROFILE_COMP_START();
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
    CPU_PROFILE_COMP_END();

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
        CPU_PROFILE_IO_START();
        while(!feof(fp) && i<40)
            indata[i++]=getc(fp);
        CPU_PROFILE_IO_END();

        CPU_PROFILE_COMP_START();
        BF_cfb64_encrypt(indata,outdata,i,&key,ivec,&num,encordec);
        CPU_PROFILE_COMP_END();

        CPU_PROFILE_IO_START();
        for(j=0;j<i;j++)
        {
            /*printf("%c",outdata[j]);*/
            fputc(outdata[j],fp2);
        }
        CPU_PROFILE_IO_END();
        i=0;
    }

    fflush(fp2);
    fclose(fp);
    fclose(fp2);
    }
    CPU_PROFILE_FLUSH_DATA();
    return 0;
}



