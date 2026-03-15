#include <string.h>

#include "flash.h"
#include "hwglue.h"

uint8_t bankFromOffset(uint32_t offset)
{
    return (offset >> 14) & FLASH_BANK_MASK;
}

uint8_t chipFromOffset(uint32_t offset)
{
    return (offset & 0x2000) ? 1 : 0;
}

uint8_t eraseSector(uint8_t nBank, uint8_t nChip)
{
    (void) nBank;
    (void) nChip;
    return 1;
}

uint8_t eraseSlot(void)
{
    return 1;
}

void __fastcall__ flashPrintVerifyError(EasyFlashAddr* pAddr,
                                        uint8_t nData,
                                        uint8_t nFlashVal)
{
    (void) pAddr;
    (void) nData;
    (void) nFlashVal;
}

uint8_t flashWrite(uint8_t nChip, uint16_t nOffset, uint8_t nVal)
{
    uint8_t* pBase = nChip ? ROM1_BASE : ROM0_BASE;
    pBase[nOffset] = nVal;
    return 1;
}

uint8_t __fastcall__ flashWriteBlock(const EasyFlashAddr* pAddr)
{
    uint8_t* pBase = pAddr->nChip ? ROM1_BASE : ROM0_BASE;
    memcpy(pBase + pAddr->nOffset, BLOCK_BUFFER, 0x100);
    return 1;
}

uint8_t __fastcall__ flashVerifyBlock(const EasyFlashAddr* pAddr)
{
    uint8_t* pBase = pAddr->nChip ? ROM1_BASE : ROM0_BASE;
    return memcmp(pBase + pAddr->nOffset, BLOCK_BUFFER, 0x100) == 0;
}

uint8_t flashWriteBankFromFile(uint8_t nBank, uint8_t nChip, uint16_t nSize)
{
    EasyFlashAddr addr;
    (void) nSize;
    addr.nSlot = 0;
    addr.nBank = nBank;
    addr.nChip = nChip;
    addr.nOffset = 0;
    return flashWriteBlock(&addr);
}
