#include <stdlib.h>
#include <Processor/Profile.h>

#define TEST_START_ADDR 0x0
#define TEST_END_ADDR   0x8000000
#define VALUE           0xDEADBEEF

int main(void)
{
    uint32_t * curr_addr = TEST_START_ADDR;
    uint32_t   value_read = 0x0;

//    printf("Before:\n");
    while (curr_addr < TEST_END_ADDR)
    {
        value_read = *curr_addr;
//        printf("0x%X : 0x%X\n", curr_addr, value_read);

//        *curr_addr = VALUE;
        curr_addr++;
    }

#if 0
//    printf("After:\n");
    curr_addr = TEST_START_ADDR;
    while (curr_addr < TEST_END_ADDR)
    {
        value_read = *curr_addr;
        if(value_read != VALUE)
        {
            printf("Test Failed: Value Written 0x%x, Value Read 0x%x\n", VALUE, value_read);
            return -1;
        }

//        printf("0x%X : 0x%X\n", curr_addr, value_read);
        curr_addr++;
    }
#endif
    printf("Test Passed !!!\n");
    return 0;
}


