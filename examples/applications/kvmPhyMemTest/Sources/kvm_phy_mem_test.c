#include <stdlib.h>
#include <Processor/Profile.h>

#define TEST_START_ADDR 0x0
#define TEST_END_ADDR   0x04000000
#define VALUE           0xDEADBEEF

int main(void)
{
    uint32_t * curr_addr = TEST_START_ADDR;
    CPU_PROFILE_ERASE_MEMORY();
    while (curr_addr < TEST_END_ADDR)
    {
        if(*curr_addr != 0x0)
        {
            printf("[%08X] = 0x%08X\n", curr_addr, *curr_addr);
            //printf("Test Failed: Value Not Zero! 0x%08X = 0x%08X\n", curr_addr, value_read);
            //return -1;
        }

        //*curr_addr = VALUE;
        curr_addr++;
    }

    //printf("Read Test Passed, Contents from 0x%X to 0x%X set to 0x%X\n", TEST_START_ADDR, TEST_END_ADDR - 4, VALUE);
    CPU_PROFILE_VERIFY_MEMORY();
	printf("Test Finished !!!\n");
    return 0;
}


