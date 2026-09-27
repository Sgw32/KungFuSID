/*
 * EasyProg - write.c - Write cartridge image to flash
 *
 * (c) 2009 Thomas Giesel
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
 *
 * Thomas Giesel skoe@directbox.com
 */

#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <peekpoke.h>

#include "cart.h"
#include "screen.h"
#include "texts.h"
#include "cart.h"
#include "kfsidprog.h"
#include "flash.h"
#include "startupbin.h"
#include "write.h"
#include "filedlg.h"
#include "sprites.h"
#include "slots.h"
#include "progress.h"
#include "timer.h"
#include "util.h"
#include "eload.h"
#include "hwglue.h"

/******************************************************************************/
/* local macros for readability */

#define EP_INTERLEAVED      1
#define EP_NON_INTERLEAVED  0

#define EF3_AR_BANK     0x10
#define EF3_SS5_BANK    0x20


#define KFSID_FW_REG                54301u
#define KFSID_FW_START_MAGIC        0xA5
#define KFSID_FW_START_ACK          0x5A
#define KFSID_FW_END_ACK            0x5A
#define KFSID_FW_SECTOR_SIZE        (16UL * 1024UL)
#define KFSID_FW_WRITE_DELAY        8u
#define KFSID_FW_DELAY_START        2000u
#define KFSID_FW_DELAY_SECTOR       3000u
#define KFSID_FW_DELAY_CHECKSUM     3000u
#define KFSID_FW_DELAY_END          4000u
#define KFSID_FW_MAX_RETRIES        3u

/******************************************************************************/
/* Static variables */

/* static to save some function call overhead */
static uint8_t  m_nBank;
static uint16_t m_nAddress;
static uint16_t m_nSize;
static BankHeader bankHeader;


static void fwDelayLoops(unsigned int loops)
{
    volatile unsigned int i;
    for (i = 0; i < loops; ++i)
    {
    }
}

static void fwWriteByte(uint8_t value)
{
    POKE(KFSID_FW_REG, value);
}

static uint8_t fwReadByte(void)
{
    return PEEK(KFSID_FW_REG);
}

static uint8_t fwStartUpdate(void)
{
    uint8_t ack;
    uint8_t attempt;

    fwWriteByte(KFSID_FW_START_MAGIC);

    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        fwDelayLoops(KFSID_FW_DELAY_START);
        ack = fwReadByte();

        if (ack == KFSID_FW_START_ACK)
            return 1;
    }

    return 0;
}

static uint8_t fwSendUpdateSector(uint8_t sector, uint8_t* pHasData)
{
    unsigned long i;
    uint16_t chunk;
    uint8_t attempt;
    uint8_t checksum;
    uint8_t devChecksum;
    int nBytes;
    uint8_t value;

    nBytes = utilRead(BLOCK_BUFFER, 0x100);
    if (nBytes <= 0)
    {
        *pHasData = 0;
        return (nBytes == 0);
    }

    if (nBytes < 0x100)
        memset(BLOCK_BUFFER + nBytes, 0xFF, 0x100 - nBytes);

    for (attempt = 0; attempt < 2; ++attempt)
    {
        fwDelayLoops(KFSID_FW_DELAY_SECTOR * 10u);
        fwWriteByte(sector);
        fwDelayLoops(KFSID_FW_DELAY_SECTOR);

        if (fwReadByte() == sector)
            break;
    }

    if (attempt == 2)
        return 0;

    checksum = 0;
    for (chunk = 0; chunk < 64; ++chunk)
    {
        if (chunk > 0)
        {
            nBytes = utilRead(BLOCK_BUFFER, 0x100);
            if (nBytes < 0)
                return 0;

            if (nBytes < 0x100)
                memset(BLOCK_BUFFER + nBytes, 0xFF, 0x100 - nBytes);
        }

        for (i = 0; i < 256UL; ++i)
        {
            value = BLOCK_BUFFER[i];
            fwWriteByte(value);
            checksum ^= value;
            fwDelayLoops(KFSID_FW_WRITE_DELAY);
        }
    }

    *pHasData = 1;

    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        devChecksum = fwReadByte();
        if (devChecksum == checksum)
            return 1;

        fwDelayLoops(KFSID_FW_DELAY_CHECKSUM);
    }

    return 0;
}

static uint8_t fwFinalizeUpdate(void)
{
    uint8_t i;
    uint8_t ack;
    uint8_t attempt;
    static const char endSequence[] = "KFSID_END";

    for (i = 0; endSequence[i] != '\0'; ++i)
    {
        fwWriteByte((uint8_t) endSequence[i]);
        fwDelayLoops(KFSID_FW_WRITE_DELAY * 8u);
    }

    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        fwDelayLoops(KFSID_FW_DELAY_END);
        ack = fwReadByte();
        if (ack == KFSID_FW_END_ACK)
            return 1;
    }

    return 0;
}

/******************************************************************************/
/**
 * Print a status line, read the next bank header from the currently active
 * input and calculate m_nBank, m_nAddress and m_nSize.
 *
 * return CART_RV_OK, CART_RV_ERR or CART_RV_EOF
 */
static uint8_t readNextHeader()
{
    uint8_t rv;

    rv = readNextBankHeader(&bankHeader);

    m_nBank = bankHeader.bank[1] & FLASH_BANK_MASK;

    m_nAddress = 256 * bankHeader.loadAddr[0] + bankHeader.loadAddr[1];
    m_nSize = 256 * bankHeader.romLen[0] + bankHeader.romLen[1];

    return rv;
}


/******************************************************************************/
/**
 * Write the startup code to 00:1:xxxx. Patch it to use the right memory
 * configuration for the present cartridge type.
 *
 * Put the bank offset to be used in *pBankOffset. This offset must be added
 * to all banks of this cartridge. This is done to keep space on bank 00:1
 * for the start up code.
 *
 * return CART_RV_OK or CART_RV_ERR
 */
static uint8_t __fastcall__ writeStartUpCode(uint8_t* pBankOffset)
{
    EasyFlashAddr addr;
    uint8_t  nConfig;

    // most CRT types are put on bank 1
    *pBankOffset = 1;

    switch (internalCartType)
    {
    case INTERNAL_CART_TYPE_NORMAL_8K:
        nConfig = EASYFLASH_IO_8K;
        break;

    case INTERNAL_CART_TYPE_NORMAL_16K:
        nConfig = EASYFLASH_IO_16K;
        break;

    case INTERNAL_CART_TYPE_ULTIMAX:
        nConfig = EASYFLASH_IO_ULTIMAX;
        break;

    case INTERNAL_CART_TYPE_OCEAN1:
        nConfig = EASYFLASH_IO_16K;
        *pBankOffset = 0;
        break;

    case INTERNAL_CART_TYPE_EASYFLASH:
        *pBankOffset = 0;
        return CART_RV_OK; // nothing to do

    case INTERNAL_CART_TYPE_EASYFLASH_XBANK:
        nConfig = nXbankConfig;
        break;

    default:
        screenPrintSimpleDialog(apStrUnsupportedCRTType);
        goto err;
    }

    // !!! keep this crap in sync with startup.s - especially the code size !!!
    // copy the startup code to the buffer and patch the start bank and config
    memcpy(BLOCK_BUFFER, startUpStart, 0x100);

    // the 1st byte of this code is the start bank to be used - patch it
    BLOCK_BUFFER[0] = *pBankOffset;

    // the 2nd byte of this code is the memory config to be used - patch it
    BLOCK_BUFFER[1] = nConfig | EASYFLASH_IO_BIT_LED;

    // write the startup code to bank 0, always write 2 * 256 bytes
    addr.nSlot = g_nSelectedSlot;
    addr.nBank = 0;
    addr.nChip = 1;
    addr.nOffset = 0x1e00;
    if (!flashWriteBlock(&addr))
        goto err;

    memcpy(BLOCK_BUFFER, startUpStart + 0x100, 0x100);
    addr.nOffset = 0x1f00;
    if (!flashWriteBlock(&addr))
    	goto err;

    // write the sprites to 00:1:1800
    // keep this in sync with sprites.s
    memcpy(BLOCK_BUFFER, pSprites, 0x100);
    addr.nOffset = 0x1800;
    if (!flashWriteBlock(&addr))
        goto err;

    memcpy(BLOCK_BUFFER, pSprites + 0x100, 0x100);
    addr.nOffset = 0x1900;
    if (!flashWriteBlock(&addr))
        goto err;

    return CART_RV_OK;
err:
	return CART_RV_ERR;
}

/******************************************************************************/
/**
 * Do all preparations to write a file to flash.
 * If pStrImageType points to "U", read the file from USB.
 *
 * If this function returns CART_RV_OK, the file has been opened successfully.
 */
static uint8_t writeOpenFile(const char* pStrImageType)
{
    uint8_t rv;

    checkFlashType();

    if (strcmp(pStrImageType, "U") == 0)
    {
        utilOpenFile(UTIL_USE_USB);
    }
    else
    {
        do
        {
            rv = fileDlg(pStrImageType);
            if (!rv)
                return CART_RV_ERR;

            rv = utilOpenFile(0);
            if (rv == 1)
                screenPrintSimpleDialog(apStrFileOpenError);
        }
        while (rv != OPEN_FILE_OK);
    }

    if (screenAskEraseDialog() != BUTTON_ENTER)
    {
        utilCloseFile();
        return CART_RV_ERR;
    }

    refreshMainScreen();
    setStatus("Checking file");

    // make sure the right areas of the chip are erased
    progressInit();
    timerStart();
    return CART_RV_OK;
}


/******************************************************************************/
/**
 * Write a cartridge image from the currently active input (file) to flash.
 *
 * return CART_RV_OK or CART_RV_ERR
 */
static uint8_t writeCrtImage(void)
{
    uint8_t rv;
    uint8_t nBankOffset;

    g_strCartName[0] = '\0';

    setStatus("Reading CRT image");
    if (!readCartHeader())
    {
        screenPrintSimpleDialog(apStrHeaderReadError);
        return CART_RV_ERR;
    }

    // this will show the cartridge type from the header
    refreshMainScreen();

    if (writeStartUpCode(&nBankOffset) != CART_RV_OK)
        return CART_RV_ERR;

    while ((rv = readNextHeader()) != CART_RV_EOF)
    {
        if (rv == CART_RV_OK)
        {
            m_nBank += nBankOffset;

            if ((m_nAddress == (uint16_t) ROM0_BASE) && (m_nSize <= 0x4000))
            {
                if (m_nSize > 0x2000)
                {
                    if (!flashWriteBankFromFile(m_nBank, 0, 0x2000) ||
                        !flashWriteBankFromFile(m_nBank, 1, m_nSize - 0x2000))
                        return CART_RV_ERR;
                }
                else
                {
                    if (!flashWriteBankFromFile(m_nBank, 0, m_nSize))
                        return CART_RV_ERR;
                }
            }
            else if (((m_nAddress == (uint16_t) ROM1_BASE) ||
                      (m_nAddress == (uint16_t) ROM1_BASE_ULTIMAX)) &&
                     (m_nSize <= 0x2000))
            {
                if (!flashWriteBankFromFile(m_nBank, 1, m_nSize))
                    return CART_RV_ERR;
            }
            else
            {
                screenPrintSimpleDialog(apStrUnsupportedCRTData);
                return CART_RV_ERR;
            }
        }
        else
        {
            screenPrintSimpleDialog(apStrChipReadError);
            return CART_RV_ERR;
        }
    }

    return CART_RV_OK;
}


/******************************************************************************/
/**
 * Write a BIN image from the given file to flash, either LOROM or HIROM,
 * beginning at nStartBank (which may have FLASH_8K_SECTOR_BIT set).
 *
 * return CART_RV_OK or CART_RV_ERR
 */
static uint8_t __fastcall__ writeBinImage(uint8_t nStartBank,
                                          uint8_t nChip,
                                          uint8_t interleaved)
{
    EasyFlashAddr addr;
    int      nBytes;
    uint8_t  pad;

    g_strCartName[0] = '\0';

    // this will show the cartridge type from the header
    refreshMainScreen();

    addr.nSlot = g_nSelectedSlot;
    addr.nBank = nStartBank;
    addr.nChip = nChip;
    addr.nOffset = 0;
    do
    {
        nBytes = utilRead(BLOCK_BUFFER, 0x100);

        if (nBytes > 0)
        {
            // the last block may be smaller than 265 bytes, pad with 0xff (unprogrammed)
            if(nBytes & 0x00ff){
                pad = nBytes;
                do{
                    BLOCK_BUFFER[pad] = 0xff;
                }while(++pad);
            }

            if (!flashWriteBlock(&addr))
                goto retError;

            if (!flashVerifyBlock(&addr))
                goto retError;

            addr.nOffset += 0x100;
            if (addr.nOffset == 0x2000)
            {
                addr.nOffset = 0;
                if (interleaved)
                {
                    if (addr.nChip == 0)
                        addr.nChip = 1;
                    else
                    {
                        addr.nChip = 0;
                        ++addr.nBank;
                    }
                }
                else
                    ++addr.nBank;
            }
        }
    }
    while (nBytes == 0x100);

    if (addr.nOffset || addr.nBank)
    {
        utilCloseFile();
        timerStop();
        return CART_RV_OK;
    }

retError:
    utilCloseFile();
    timerStop();
    return CART_RV_ERR;
}


/******************************************************************************/
/**
 * Write a CRT image file to flash. This version doesn't need any user
 * interaction. g_strFileName must contain the file name already.
 *
 * Return 1 if everything worked well.
 */
uint8_t autoWriteCRTImage(uint8_t nSlot)
{
    uint8_t rv;

    refreshMainScreen();

    slotSelect(nSlot);
    rv = utilOpenFile(0);
    if (rv == 1)
        return 0;

    // make sure the right areas of the chip are erased
    progressInit();

    rv = writeCrtImage();
    utilCloseFile();

    if (rv == CART_RV_OK)
    {
        if (g_nSlots > 1 && g_nSelectedSlot != 0)
        {
            slotSaveName(g_strCartName, ~0);
        }
    }
    return 1;
}


/******************************************************************************/
/**
 * Write a CRT image file to flash.
 */
void checkWriteCRTImage(void)
{
    uint8_t rv;

    if (checkAskForSlot() && (writeOpenFile("CRT") == CART_RV_OK))
    {
        rv = writeCrtImage();
        utilCloseFile();
        timerStop();

        if (rv == CART_RV_OK)
        {
            if (g_nSlots > 1 && g_nSelectedSlot != 0)
            {
                slotSaveName(screenReadInput("Cartridge Name", g_strCartName),
                    ~0);
            }
            screenPrintSimpleDialog(apStrWriteComplete);
        }
    }
}


/******************************************************************************/
/**
 * todo: merge functions!
 */
void checkWriteCRTImageFromUSB(void)
{
    uint8_t rv;

    if (checkAskForSlot() && (writeOpenFile("U") == CART_RV_OK))
    {
        rv = writeCrtImage();
        utilCloseFile();
        timerStop();

        if (rv == CART_RV_OK)
        {
            if (g_nSlots > 1 && g_nSelectedSlot != 0)
            {
                slotSaveName(screenReadInput("Cartridge Name", g_strCartName),
                    ~0);
            }
            screenPrintSimpleDialog(apStrWriteComplete);
        }
    }
}



/******************************************************************************/
/**
 * Load firmware .BIN file and flash it using the updater protocol.
 */
void checkWriteUpdateBIN(void)
{
    uint8_t sector = 0;

    if (writeOpenFile("BIN") != CART_RV_OK)
        return;

    setStatus("Starting updater protocol");

    if (!fwStartUpdate())
    {
        utilCloseFile();
        timerStop();
        screenPrintSimpleDialog(apStrFlashWriteFailed);
        return;
    }

    while (1)
    {
        uint8_t hasData;

        hasData = 0;
        setStatus("Flashing updater sector");
        if (!fwSendUpdateSector(sector, &hasData))
        {
            utilCloseFile();
            timerStop();
            screenPrintSimpleDialog(apStrFlashWriteFailed);
            return;
        }

        if (!hasData)
            break;

        ++sector;
    }

    utilCloseFile();
    timerStop();

    if (!fwFinalizeUpdate())
    {
        screenPrintSimpleDialog(apStrFlashWriteFailed);
        return;
    }

    screenPrintSimpleDialog(apStrWriteComplete);
}

/******************************************************************************/
/**
 * Write a BIN image file to the LOROM flash.
 */
void checkWriteLOROMImage(void)
{
    uint8_t rv;

    if (checkAskForSlot() && (writeOpenFile("BIN") == CART_RV_OK))
    {
        rv = writeBinImage(0, 0, EP_NON_INTERLEAVED);
        if (rv == CART_RV_OK)
            screenPrintSimpleDialog(apStrWriteComplete);
    }
}


/******************************************************************************/
/**
 * Write a BIN image file to the HIROM flash.
 */
void checkWriteHIROMImage(void)
{
    uint8_t rv;

    if (checkAskForSlot() && (writeOpenFile("BIN") == CART_RV_OK))
    {
        rv = writeBinImage(0, 1, EP_NON_INTERLEAVED);
        if (rv == CART_RV_OK)
            screenPrintSimpleDialog(apStrWriteComplete);
    }
}


/******************************************************************************/
/**
 * Write a KERNAL image file to the flash.
 */
void checkWriteKERNALImage(void)
{
    uint8_t nKERNAL, rv;

    slotSelect(0);
    nKERNAL = selectKERNALSlotDialog();
    if (nKERNAL != 0xff)
    {
        if (writeOpenFile("BIN") == CART_RV_OK)
        {
            rv = writeBinImage(nKERNAL | FLASH_8K_SECTOR_BIT, 0,
                               EP_NON_INTERLEAVED);
            if (rv == CART_RV_OK)
            {
                slotSaveName(screenReadInput("KERNAL Name", g_strFileName),
                             nKERNAL);
                screenPrintSimpleDialog(apStrWriteComplete);
            }
        }
    }
}


/******************************************************************************/
/**
 * Write a AR/RR/NP image file to the flash.
 */
void checkWriteARImage(void)
{
    uint8_t nAR, rv;

    slotSelect(0);
    nAR = selectARSlotDialog();
    if (nAR != 0xff)
    {
        if (writeOpenFile("BIN") == CART_RV_OK)
        {
            rv = writeBinImage(nAR * 8 + EF3_AR_BANK, 1, EP_NON_INTERLEAVED);
            if (rv == CART_RV_OK)
                screenPrintSimpleDialog(apStrWriteComplete);
        }
    }
}


/******************************************************************************/
/**
 * Write a SS5 image file to the flash.
 */
void checkWriteSS5Image(void)
{
    uint8_t rv;

    slotSelect(0);
    if (writeOpenFile("BIN") == CART_RV_OK)
    {
        rv = writeBinImage(EF3_SS5_BANK, 0, EP_INTERLEAVED);
        if (rv == CART_RV_OK)
            screenPrintSimpleDialog(apStrWriteComplete);
    }
}


/******************************************************************************/
/**
 */
void eraseAll(void)
{
    uint8_t i;

    checkFlashType();
    for (i = 0; i < g_nSlots; ++i)
    {
        slotSelect(i);
        eraseSlot();
    }
    resetCartInfo();
}


/******************************************************************************/
/**
 * Ask the user if it is okay to erase all and do so if yes.
 */
void checkEraseAll(void)
{
    if (screenAskEraseDialog() == BUTTON_ENTER)
        eraseAll();
}


/******************************************************************************/
/**
 * Ask the user if it is okay to erase a slot and do so if yes.
 */
void checkEraseSlot(void)
{
    if (g_nSlots > 1)
    {
        if (!checkAskForSlot())
            return;
    }

    if (screenAskEraseDialog() == BUTTON_ENTER)
    {
        checkFlashType();
        eraseSlot();

        if (g_nSelectedSlot > 0)
        {
            strcpy(utilStr, "Slot ");
            utilAppendDecimal(g_nSelectedSlot);
            slotSaveName(utilStr, 0xff);
        }
        resetCartInfo();
    }
}


/******************************************************************************/
/**
 * Ask the user if it is okay to erase a KERNAL and do so if yes.
 */
void checkEraseKERNAL(void)
{
    uint8_t nKERNAL;

    slotSelect(0);
    nKERNAL = selectKERNALSlotDialog();
    if (nKERNAL != 0xff)
    {
        if (screenAskEraseDialog() == BUTTON_ENTER)
        {
            checkFlashType();
            eraseSector(nKERNAL | FLASH_8K_SECTOR_BIT, 0);
            strcpy(utilStr, "KERNAL ");
            utilAppendDecimal(nKERNAL + 1);
            slotSaveName(utilStr, nKERNAL);
            resetCartInfo();
        }
    }
}

/******************************************************************************/
/**
 */
void checkEraseAR(void)
{
    uint8_t nAR;

    slotSelect(0);
    nAR = selectARSlotDialog();
    if (nAR != 0xff)
    {
        if (screenAskEraseDialog() == BUTTON_ENTER)
        {
            checkFlashType();
            eraseSector(nAR * 8 + EF3_AR_BANK, 1);
            resetCartInfo();
        }
    }
}


/******************************************************************************/
/**
 */
void checkEraseSS5(void)
{
    slotSelect(0);
    if (screenAskEraseDialog() == BUTTON_ENTER)
    {
        checkFlashType();
        eraseSector(EF3_SS5_BANK, 0);
        eraseSector(EF3_SS5_BANK, 1);
        resetCartInfo();
    }
}
