/*
 *  Copied and modified from linux (arch/x86/boot/main.c)
 */

/*
 * Main module for the real-mode kernel code
 */

#include "boot.h"

static void keyboard_set_repeat (void)
{
    struct biosregs ireg;
    initregs(&ireg);
    ireg.ax = 0x0305;
    intcall (0x16, &ireg, NULL);
}

void main(void)
{
    puts ("main bootstrap\n");

    /* Set keyboard repeat rate */
    keyboard_set_repeat();

    /* Do the last things and invoke protected mode */
    go_to_protected_mode();
}

