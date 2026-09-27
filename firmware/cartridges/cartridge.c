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

#include "cartridge.h"

// Current ROM or RAM bank pointer
static u8 *crt_ptr;

                      
/* ordered by cartridge id */
#include "crt_normal.c"
#include "kff.c"

#define TIMING_HANDLER(name)                                                \
    (c64_timing_get() == C64_TIMING_NTSC ? name##_ntsc_handler :            \
     (c64_timing_get() == C64_TIMING_C16 ? name##_c16_handler :             \
                                          name##_pal_handler))

static void (*crt_get_handler(u32 cartridge_type, bool vic_support)) (void)
{
    

    return NULL;
}

static void crt_init(DAT_CRT_HEADER *crt_header)
{
    switch (crt_header->type)
    {
        
    }
}

static void crt_install_handler(DAT_CRT_HEADER *crt_header)
{
    u32 state = STATUS_LED_ON;
    if (!(crt_header->type & CRT_C128_CARTRIDGE))
    {
        if (crt_header->exrom)
        {
            state |= C64_EXROM_HIGH;
        }
        else
        {
            state |= C64_EXROM_LOW;
        }

        if (crt_header->game)
        {
            state |= C64_GAME_HIGH;
        }
        else
        {
            state |= C64_GAME_LOW;
        }
    }
    else
    {
        state |= CRT_PORT_NONE;
    }

    C64_CRT_CONTROL(state);

    crt_ptr = crt_banks[0];
    crt_init(crt_header);

    u32 cartridge_type = crt_header->type;
    bool vic_support = (crt_header->flags & CRT_FLAG_VIC) != 0;
    void (*handler)(void) = crt_get_handler(cartridge_type, vic_support);
    C64_INSTALL_HANDLER(handler);
}

static bool crt_is_supported(u32 cartridge_type)
{
    return crt_get_handler(cartridge_type, false) != NULL;
}
