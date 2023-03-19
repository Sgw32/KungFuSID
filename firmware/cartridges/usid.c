#include "usid.h"

#include "irq.c"

void setup() {

    InitHardware(); // 2_setup.ino  (start SID emulator)
    reset_SID(); // in 6_barebone_sounds.ino
}

void delay(int i)
{

}

