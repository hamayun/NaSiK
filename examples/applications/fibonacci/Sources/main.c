#include <stdio.h>
#include <Processor/Profile.h>

#define ENABLE_IO 0
#define PRINTF if(ENABLE_IO) printf

long int fibonacci(int index)
{
    if(index <= 2)
        return 1;
    else
        return(fibonacci(index - 1) + fibonacci(index - 2));
}

int main(void) 
{
    int index = 30;
    CPU_PROFILE_COMP_START();
    //for(index = 1; index < 30; index++)
    {
        long fibo = fibonacci(index);
        PRINTF("Fibonacci Index (%2d): %ld\n", index, fibo);
    }
    CPU_PROFILE_COMP_END();
    CPU_PROFILE_FLUSH_DATA();
    return 0;
}
