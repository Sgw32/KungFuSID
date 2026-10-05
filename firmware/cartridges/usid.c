#include "usid.h"

#include "irq.c"
#include "pot.c"

void setup(void)
{
    InitHardware(); // Start the SID-compatible emulator hardware.
    emulator_backend_reset();
}

void delay(int i)
{
    (void)i;
}

