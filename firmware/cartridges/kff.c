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
#include "sid.h"
#define KFF_BUF (CRT_DAT_BANK(0))
#define KFF_RAM (CRT_RAM_BUF)
#define KFF_ID_VALUE 0x2a

// $de01 Command register in KFF RAM
#define KFF_COMMAND (*((volatile u8*)(KFF_RAM + 1)))

// $de04-$de05 Read buffer pointer register in KFF RAM
#define KFF_READ_PTR (*((u16*)(KFF_RAM + 4)))

// $de06-$de07 Write buffer pointer register in KFF RAM
#define KFF_WRITE_PTR (*((u16*)(KFF_RAM + 6)))

// $de09 ID register in KFF RAM (same address as EF3 USB Control register)
#define KFF_ID (*((u8*)(KFF_RAM + 9)))


uint32_t data_buffer[255];
uint32_t address_buffer[255];
uint8_t	readIndex=0;
uint8_t	writeIndex=0;
uint8_t	bufferLength=0;

/*************************************************
* C64 bus read callback
*************************************************/
FORCE_INLINE bool kff_read_handler(u32 control, u32 addr)
{
    if (!(addr&0b100000))
    {
	    C64_DATA_WRITE(SID[addr&0b11111]);
        return true;
    }
    return false;
}

/*************************************************
* C64 bus write callback
*************************************************/
FORCE_INLINE void kff_write_handler(u32 control, u32 addr, u32 data)
{
        /*switch (addr)
        {
            case 0xd400:
                led_toggle();
                break;
            default:    // RAM at rest of $de00-$deff
               break;                
        } */
	if (!(addr&0b100000))
	{
            //led_toggle();
            led_toggle();
            setreg(addr&0b11111,data);
            //address_buffer[writeIndex] = addr;
	        //data_buffer[writeIndex]=data;
	        //writeIndex++;
	        //bufferLength++;
	}
}

static void kff_init(void)
{
    C64_CRT_CONTROL(STATUS_LED_ON|CRT_PORT_NONE);
    KFF_ID = KFF_ID_VALUE;
}

// Allow SDIO and USB to be used while handling C64 bus access.
C64_BUS_HANDLER(kff)
