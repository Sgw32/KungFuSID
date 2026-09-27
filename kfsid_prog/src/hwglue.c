#include <string.h>

#include "flash.h"
#include "hwglue.h"

static uint8_t g_bank;
static uint8_t g_slot;

uint8_t efShowROM(void* addr)
{
    (void) addr;
    return 1;
}

uint8_t efHideROM(void* addr)
{
    (void) addr;
    return 1;
}

uint8_t __fastcall__ efPeekCartROM(void* addr)
{
    return *((uint8_t*) addr);
}

void* __fastcall__ efCopyCartROM(void* dest, const void* src, size_t count)
{
    return memcpy(dest, src, count);
}

uint8_t __fastcall__ hwInit(uint8_t* pManufacturerId, uint8_t* pDeviceId)
{
    if (pManufacturerId)
        *pManufacturerId = FLASH_MX29LV640EB_MFR_ID;
    if (pDeviceId)
        *pDeviceId = FLASH_MX29LV640EB_DEV_ID;
    return FLASH_NUM_BANKS;
}

uint8_t hwReInit(void)
{
    return 1;
}

uint8_t __fastcall__ hwGetBank(void)
{
    return g_bank;
}

void __fastcall__ hwSetBank(uint8_t nBank)
{
    g_bank = nBank;
}

uint8_t __fastcall__ hwSectorErase(uint8_t* pBase)
{
    (void) pBase;
    return 1;
}

uint8_t __fastcall__ hwWriteBlock(uint8_t* pDst)
{
    memcpy(pDst, BLOCK_BUFFER, 0x100);
    return 1;
}

uint8_t __fastcall__ hwGetSlot(void)
{
    return g_slot;
}

void __fastcall__ hwSetSlot(uint8_t nSlot)
{
    g_slot = nSlot;
}
