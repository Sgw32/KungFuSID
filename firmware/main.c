/*
 * Copyright (c) 2019-2022 Kim Jørgensen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "memory.h"
#include "hal.c"
#include "print.c"     
#include "file_types.h"
#include "cartridge.c"

int main(void)
{
    configure_system();
    //c64_launcher_mode();
    crt_ptr = CRT_LAUNCHER_BANK;
    kff_init();
    C64_INSTALL_HANDLER(kff_handler);
    c64_enable();

    while (true)
    {

    }
}
