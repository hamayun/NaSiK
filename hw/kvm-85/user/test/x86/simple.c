//#include <stdio.h>

static void print_lll(const char *buf)
{
	unsigned long len = 21;

	asm volatile ("rep/outsb" 
								: "+S"(buf), "+c"(len) 
								: "d"(0xf1)
							 );
}

int system_kickstart (void)
{
		print_lll("MMH:\n");
		return 0;
}

int main()
{
	//long result = 0;
	//long i;
	//for(i=0; i<1000000; i++)
	//	asm volatile ("outsb $0x50, 0xf1");

	char *s = "Hello, World!\n";
	//s[0] = 'H';
	//s[1] = 'e';
	//s[2] = 'l';
	//s[3] = 'l';
	//s[4] = 0;
	print_lll(s);
	//printf("by lib\n");
	return 0;
}
