
#include <Processor/Profile.h>

int main(void)
{
    int count = 0;

    CPU_PROFILE_CURRENT_TIME() ;
    for(count = 0; count < 1000000; count++)
    {
        CPU_PROFILE_COMP_START(); CPU_PROFILE_COMP_END();
    }
    CPU_PROFILE_CURRENT_TIME();

    CPU_PROFILE_FLUSH_DATA();
    return 0;
}
/*
Example Output:
===============
soc_kvm_run: Calling kvm_run ...
Current Time is : 0.224014
Current Time is : 22.213388
Total I/O Time              : 0.000000    [000000/000000] Profile Overhead: 0.000000, Net Cost: 0.000000
Total Computation Time      : 11.140699    [1000000/1000000] Profile Overhead: 21.910000, Net Cost: -10.769301
Total I/O + Computation Time: 11.140699    [1000000/1000000] Profile Overhead: 21.910000, Net Cost: -10.769301
*/

/*
So the Total Time it took to Execute = 22.213388 - 0.224014
                                     = 21.989374 Seconds
Per Profile Call Cost = (21.989374 / (1000000 + 1000000))
                      = 0.000010995 Seconds
Reported Profile Overhead = 21.910000 Seconds. 
Error = ((21.989374 - 21.910000) / 21.989374) * 100
      = 0.360965255 Percent
*/
