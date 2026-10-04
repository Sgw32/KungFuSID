/*
 * KungFuSID Programmer
 *
 * Based on EasyProg, (c) 2009-2012 Thomas Giesel.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented.
 * 2. Altered source versions must be plainly marked as such.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "filedlg.h"
#include "hwglue.h"
#include "kfsidprog.h"
#include "params.h"
#include "progress.h"
#include "screen.h"
#include "texts.h"
#include "util.h"
#include "write.h"

#define KFSID_FW_REG                54301u
#define KFSID_FW_START_MAGIC        0xA5
#define KFSID_FW_START_ACK          0x5A
#define KFSID_FW_END_ACK            0xE5
#define KFSID_FW_SECTOR_SIZE        (16UL * 1024UL)
#define KFSID_FW_MAX_SIZE           (64UL * 1024UL)
#define KFSID_FW_WRITE_DELAY        8u
#define KFSID_FW_DELAY_START        2000u
#define KFSID_FW_DELAY_SECTOR       3000u
#define KFSID_FW_DELAY_CHECKSUM     3000u
#define KFSID_FW_DELAY_END          4000u
#define KFSID_FW_MAX_RETRIES        3u
#define KFSID_FW_CHUNKS_PER_SECTOR  64u

static char fileVersion[17] = "--";
static unsigned long fileSize;
static uint8_t fileSectors;

const char* firmwareFileVersion(void)
{
    return fileVersion;
}

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
    uint8_t attempt;

    fwWriteByte(KFSID_FW_START_MAGIC);
    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        fwDelayLoops(KFSID_FW_DELAY_START);
        if (fwReadByte() == KFSID_FW_START_ACK)
            return 1;
    }
    return 0;
}

static uint8_t fwSendUpdateSector(uint8_t sector)
{
    uint16_t chunk;
    uint16_t i;
    uint8_t attempt;
    uint8_t checksum = 0;
    int nBytes;

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

    for (chunk = 0; chunk < KFSID_FW_CHUNKS_PER_SECTOR; ++chunk)
    {
        nBytes = utilRead(BLOCK_BUFFER, 0x100);
        if (nBytes < 0)
            return 0;
        if (nBytes < 0x100)
            memset(BLOCK_BUFFER + nBytes, 0xFF, 0x100 - nBytes);

        for (i = 0; i < 0x100; ++i)
        {
            checksum ^= BLOCK_BUFFER[i];
            fwWriteByte(BLOCK_BUFFER[i]);
            fwDelayLoops(KFSID_FW_WRITE_DELAY);
        }

        progressSet((uint16_t)(sector * KFSID_FW_CHUNKS_PER_SECTOR + chunk + 1),
                    (uint16_t)(fileSectors * KFSID_FW_CHUNKS_PER_SECTOR));
    }

    /* Sector erase/program runs synchronously on KungFuSID. */
    for (i = 0; i < 90; ++i)
        waitvsync();

    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        if (fwReadByte() == checksum)
            return 1;
        fwDelayLoops(KFSID_FW_DELAY_CHECKSUM);
    }
    return 0;
}

static uint8_t fwFinalizeUpdate(void)
{
    uint8_t i;
    uint8_t attempt;
    static const char endSequence[] = "KFSID_END";

    for (i = 0; endSequence[i] != '\0'; ++i)
    {
        fwWriteByte((uint8_t)endSequence[i]);
        fwDelayLoops(KFSID_FW_WRITE_DELAY * 8u);
    }

    for (attempt = 0; attempt < KFSID_FW_MAX_RETRIES; ++attempt)
    {
        fwDelayLoops(KFSID_FW_DELAY_END);
        if (fwReadByte() == KFSID_FW_END_ACK)
            return 1;
    }
    return 0;
}

static uint8_t inspectOpenFirmware(void)
{
    static const char marker[] = "KFSIDFW:";
    uint8_t markerIndex = 0;
    uint8_t versionIndex = 0;
    uint8_t capturing = 0;
    uint8_t found = 0;
    uint16_t i;
    int nBytes;

    fileSize = 0;
    fileVersion[0] = '\0';

    while ((nBytes = utilRead(BLOCK_BUFFER, 0x100)) > 0)
    {
        fileSize += (unsigned int)nBytes;
        if (fileSize > KFSID_FW_MAX_SIZE)
            return 0;

        for (i = 0; i < (uint16_t)nBytes; ++i)
        {
            uint8_t value = BLOCK_BUFFER[i];

            if (capturing)
            {
                if (value == 0)
                {
                    fileVersion[versionIndex] = '\0';
                    found = versionIndex != 0;
                    capturing = 0;
                }
                else if (versionIndex < sizeof(fileVersion) - 1)
                {
                    fileVersion[versionIndex++] = (char)value;
                }
                else
                {
                    return 0;
                }
            }
            else if (!found)
            {
                if (value == (uint8_t)marker[markerIndex])
                {
                    if (++markerIndex == sizeof(marker) - 1)
                    {
                        capturing = 1;
                        markerIndex = 0;
                    }
                }
                else
                {
                    markerIndex = value == (uint8_t)marker[0] ? 1 : 0;
                }
            }
        }
    }

    if (nBytes < 0 || fileSize == 0)
        return 0;

    /* Images produced before version metadata was introduced remain
     * flashable; the UI makes the missing version explicit. */
    if (!found)
        strcpy(fileVersion, "Unknown");

    fileSectors = (uint8_t)((fileSize + KFSID_FW_SECTOR_SIZE - 1) /
                            KFSID_FW_SECTOR_SIZE);
    return fileSectors > 0 && fileSectors <= 4;
}

static uint8_t selectFirmwareFile(void)
{
    uint8_t rv;

    do
    {
        if (!fileDlg("BIN"))
            return 0;
        rv = utilOpenFile(0);
        if (rv != OPEN_FILE_OK)
            screenPrintSimpleDialog(apStrFileOpenError);
    }
    while (rv != OPEN_FILE_OK);

    setStatus("Inspecting firmware image");
    if (!inspectOpenFirmware())
    {
        utilCloseFile();
        strcpy(fileVersion, "Invalid");
        refreshMainScreen();
        screenPrintSimpleDialog(apStrInvalidFirmware);
        return 0;
    }
    utilCloseFile();
    refreshMainScreen();
    return 1;
}

static uint8_t confirmFirmwareUpdate(void)
{
    const char* lines[7];
    char fileLine[32];
    char installedLine[32];

    strcpy(fileLine, "File version: ");
    strcat(fileLine, fileVersion);
    strcpy(installedLine, "Installed: ");
    strcat(installedLine, g_kfsidInstalledVersion);

    lines[0] = "Update KungFuSID firmware?";
    lines[1] = fileLine;
    lines[2] = installedLine;
    lines[3] = "Do not switch off the C64.";
    lines[4] = "<Stop> cancels, <Enter> updates.";
    lines[5] = NULL;
    return screenPrintDialog(lines, BUTTON_ENTER | BUTTON_STOP) == BUTTON_ENTER;
}

void checkWriteUpdateBIN(void)
{
    uint8_t sector;
    uint8_t i;

    if (!selectFirmwareFile() || !confirmFirmwareUpdate())
        return;

    if (utilOpenFile(0) != OPEN_FILE_OK)
    {
        screenPrintSimpleDialog(apStrFileOpenError);
        return;
    }

    progressInit();
    refreshMainScreen();
    setStatus("Starting updater protocol");
    if (!fwStartUpdate())
        goto failed;

    for (sector = 0; sector < fileSectors; ++sector)
    {
        setStatus("Programming KungFuSID firmware");
        if (!fwSendUpdateSector(sector))
            goto failed;
    }

    utilCloseFile();
    setStatus("Finalizing firmware update");
    if (!fwFinalizeUpdate())
        goto failed_closed;

    /* KungFuSID reboots after acknowledging the final command. */
    for (i = 0; i < 60; ++i)
        waitvsync();
    (void)refreshDeviceInfo();
    refreshMainScreen();
    screenPrintSimpleDialog(apStrWriteComplete);
    return;

failed:
    utilCloseFile();
failed_closed:
    strcpy(g_kfsidDeviceState, "Update failed");
    refreshMainScreen();
    screenPrintSimpleDialog(apStrFlashWriteFailed);
}
