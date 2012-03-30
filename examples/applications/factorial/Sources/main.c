#include <stdio.h>
#include <Processor/Profile.h>

unsigned long factorial(int x)
{
    if(x == 1)
        return x;
    else
        return (x * factorial(x - 1));
}

int main(void) {
        int i = 0;
        unsigned long f = 0;
        CPU_PROFILE_COMP_START();
        for (; i < 200000; i++)
        {
            f = factorial(14);
        }

//        printf("Factorial is = %ld\n", f);
        CPU_PROFILE_COMP_END();
        CPU_PROFILE_FLUSH_DATA();
        return 0;
}

#if 0
int main(void) {
        long f, sum = 0x0;
        int i = 4;

        //for(i = 1; i < 10; i++)
        {
                f = factorial(i);
                sum += f;
                printf("Factorial of %d = %ld\n", i, f);
        }

        printf("Sum of Factorial's = %ld [0x%lX]\n", sum, sum);
        return sum;      // sum = 0x63E19
}

int main(void) 
{
	int i = 1;

//	for(; i <= 515; i++)
		printf("M");
    printf("\n");

//    printf("MESSAGE0 0");

    return 0;
}
#endif

