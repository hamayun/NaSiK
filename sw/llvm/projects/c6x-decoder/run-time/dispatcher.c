
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#if 0
extern int simulated_bb();

int main(int argc, char **argv, char **environ)
{
    CPU_PROFILE_VERIFY_MEMORY();

    simulated_bb();

    while (1);
    return 0;
}

#else

extern int InitProcessorState();
extern int ShowProcessorState();

extern int Simulate_EP00000000();
extern int Simulate_EP00000004();
extern int Simulate_EP0000000c();
extern int Simulate_EP00000010();
extern int Simulate_EP00000014();
extern int Simulate_EP00000018();
extern int Simulate_EP0000001c();

int main(int argc, char **argv, char **environ)
{
    CPU_PROFILE_VERIFY_MEMORY();

    InitProcessorState();

    Simulate_EP00000000();
    Simulate_EP00000004();
    Simulate_EP0000000c();
    Simulate_EP00000010();
    Simulate_EP00000014();
    Simulate_EP00000018();
    Simulate_EP0000001c();

    ShowProcessorState();
    while (1);
    return 0;
}
#endif
