/* KungFuSID Amiga sample-bank loader. */
#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bank.h"
#include "filedlg.h"
#include "hwglue.h"
#include "kfsidprog.h"
#include "params.h"
#include "progress.h"
#include "screen.h"
#include "texts.h"
#include "util.h"

#define BANK_REG          54301u
#define BANK_START        0xB1
#define BANK_START_ACK    0x1B
#define BANK_READY        0xB2
#define BANK_END          0xB3
#define BANK_END_ACK      0x3B
#define BANK_ERROR        0xBF
#define BANK_MAX_BYTES    (384UL * 1024UL)
#define BANK_WRITE_DELAY  8u

static unsigned long bankSize;
static uint8_t bankCount;
static char bankName[17];

static uint16_t le16(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t crcByte(uint32_t crc, uint8_t value)
{
    static const uint32_t nibble[16] = {
        0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL,
        0x76DC4190UL, 0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL,
        0xEDB88320UL, 0xF00F9344UL, 0xD6D6A3E8UL, 0xCB61B38CUL,
        0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL, 0xBDBDF21CUL
    };
    crc ^= value;
    crc = (crc >> 4) ^ nibble[crc & 15U];
    crc = (crc >> 4) ^ nibble[crc & 15U];
    return crc;
}

static void bankDelay(unsigned int loops)
{
    volatile unsigned int i;
    for (i = 0; i < loops; ++i) { }
}

static uint8_t bankWait(uint8_t expected, uint16_t frames)
{
    uint8_t value;
    while (frames--)
    {
        value = PEEK(BANK_REG);
        if (value == expected) return 1;
        if (value == BANK_ERROR) return 0;
        waitvsync();
    }
    return 0;
}

static uint8_t inspectBank(void)
{
    uint8_t header[64];
    uint32_t position = 0;
    uint32_t dirOffset;
    uint32_t pcmOffset;
    uint32_t dirLength;
    uint32_t expectedDirCrc;
    uint32_t expectedPcmCrc;
    uint32_t dirCrc = 0xFFFFFFFFUL;
    uint32_t pcmCrc = 0xFFFFFFFFUL;
    uint16_t i;
    int count;

    count = utilRead(header, sizeof(header));
    if (count != sizeof(header) || memcmp(header, "KFSAMP1\0", 8) ||
        le16(header + 8) != 1 || le16(header + 10) != 64 ||
        le16(header + 20) == 0 || le16(header + 20) > 64 ||
        le16(header + 22) != 64 || le32(header + 60) != 0x4B53424BUL)
        return 0;
    bankSize = le32(header + 16);
    bankCount = (uint8_t)le16(header + 20);
    dirOffset = le32(header + 24);
    pcmOffset = le32(header + 28);
    dirLength = 64UL * 64UL;
    expectedDirCrc = le32(header + 32);
    expectedPcmCrc = le32(header + 36);
    if (bankSize < 4352UL || bankSize > BANK_MAX_BYTES ||
        dirOffset < 64UL || dirOffset + dirLength > pcmOffset || pcmOffset > bankSize)
        return 0;
    memcpy(bankName, header + 44, 16);
    bankName[16] = '\0';

    for (i = 0; i < sizeof(header); ++i, ++position)
    {
        if (position >= dirOffset && position < dirOffset + dirLength)
            dirCrc = crcByte(dirCrc, header[i]);
        if (position >= pcmOffset)
            pcmCrc = crcByte(pcmCrc, header[i]);
    }
    while ((count = utilRead(BLOCK_BUFFER, 256)) > 0)
    {
        for (i = 0; i < (uint16_t)count; ++i, ++position)
        {
            if (position >= dirOffset && position < dirOffset + dirLength)
                dirCrc = crcByte(dirCrc, BLOCK_BUFFER[i]);
            if (position >= pcmOffset && position < bankSize)
                pcmCrc = crcByte(pcmCrc, BLOCK_BUFFER[i]);
        }
        if (position > bankSize) return 0;
    }
    return count == 0 && position == bankSize &&
           (dirCrc ^ 0xFFFFFFFFUL) == expectedDirCrc &&
           (pcmCrc ^ 0xFFFFFFFFUL) == expectedPcmCrc;
}

static uint8_t chooseAndInspect(void)
{
    uint8_t result;
    do
    {
        if (!fileDlg("KSB")) return 0;
        result = utilOpenFile(0);
        if (result != OPEN_FILE_OK) screenPrintSimpleDialog(apStrFileOpenError);
    } while (result != OPEN_FILE_OK);
    setStatus("Checking Amiga sample bank");
    result = inspectBank();
    utilCloseFile();
    if (!result)
    {
        static const char* invalid[] = {
            "Invalid KungFuSID sample bank.",
            "The file or its CRC is damaged.", NULL };
        screenPrintSimpleDialog(invalid);
    }
    return result;
}

static uint8_t confirmBank(void)
{
    const char* lines[7];
    char nameLine[35];
    char countLine[25];
    strcpy(nameLine, "Bank: ");
    strncat(nameLine, bankName, 27);
    sprintf(countLine, "Instruments: %u", bankCount);
    lines[0] = "Flash Amiga sample bank?";
    lines[1] = nameLine;
    lines[2] = countLine;
    lines[3] = "The previous bank stays safe.";
    lines[4] = "<Stop> cancels, <Enter> flashes.";
    lines[5] = NULL;
    return screenPrintDialog(lines, BUTTON_ENTER | BUTTON_STOP) == BUTTON_ENTER;
}

void flashAmigaSampleBank(void)
{
    uint32_t remaining;
    uint16_t chunks;
    uint16_t chunk;
    uint16_t i;
    uint8_t checksum;
    uint8_t sizeByte;
    int count;
    static const char* done[] = {
        "Amiga sample bank installed.",
        "KungFuSID restarted successfully.", NULL };

    if (!chooseAndInspect() || !confirmBank()) return;
    if (utilOpenFile(0) != OPEN_FILE_OK)
    {
        screenPrintSimpleDialog(apStrFileOpenError);
        return;
    }
    progressInit();
    refreshMainScreen();
    setStatus("Starting sample bank update");
    POKE(BANK_REG, BANK_START);
    if (!bankWait(BANK_START_ACK, 60)) goto failed;
    for (i = 0; i < 4; ++i)
    {
        sizeByte = (uint8_t)(bankSize >> (i * 8));
        POKE(BANK_REG, sizeByte);
        bankDelay(BANK_WRITE_DELAY);
    }
    setStatus("Erasing inactive bank slot");
    if (!bankWait(BANK_READY, 360)) goto failed;

    remaining = bankSize;
    chunks = (uint16_t)((bankSize + 255UL) / 256UL);
    setStatus("Flashing Amiga sample bank");
    for (chunk = 0; chunk < chunks; ++chunk)
    {
        count = utilRead(BLOCK_BUFFER, 256);
        if (count < 0 || (count == 0 && remaining)) goto failed;
        if (count < 256) memset(BLOCK_BUFFER + count, 0xFF, 256 - count);
        checksum = 0;
        for (i = 0; i < 256; ++i)
        {
            checksum ^= BLOCK_BUFFER[i];
            POKE(BANK_REG, BLOCK_BUFFER[i]);
            bankDelay(BANK_WRITE_DELAY);
        }
        if (!bankWait(checksum, 120)) goto failed;
        remaining -= remaining < 256UL ? remaining : 256UL;
        progressSet((uint16_t)(chunk + 1), chunks);
    }
    utilCloseFile();
    setStatus("Verifying sample bank");
    POKE(BANK_REG, BANK_END);
    if (!bankWait(BANK_END_ACK, 360)) goto failedClosed;
    for (i = 0; i < 90; ++i) waitvsync();
    (void)refreshDeviceInfo();
    refreshMainScreen();
    screenPrintSimpleDialog(done);
    return;

failed:
    utilCloseFile();
failedClosed:
    setStatus("Sample bank update failed");
    screenPrintSimpleDialog(apStrFlashWriteFailed);
}
