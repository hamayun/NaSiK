
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

extern int simulated_bb();

int main(int argc, char **argv, char **environ)
{
    CPU_PROFILE_VERIFY_MEMORY();

    simulated_bb();

    while (1);
    return 0;
}

