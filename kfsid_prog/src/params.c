#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "screen.h"

#define KFSID_PROTOCOL_REG              54301u /* $D41D */
#define KFSID_PARAM_START_MAGIC         0xC1
#define KFSID_PARAM_START_ACK           0x1C
#define KFSID_PARAM_END_ACK             0xE1
#define KFSID_PARAM_CMD_GET_COUNT       0x10
#define KFSID_PARAM_CMD_GET_VALUE       0x11
#define KFSID_PARAM_CMD_SET_VALUE       0x12
#define KFSID_PARAM_CMD_SAVE            0x13
#define KFSID_PARAM_CMD_LOAD_DEFAULT    0x14
#define KFSID_PARAM_CMD_GET_INFO        0x15
#define KFSID_PARAM_CMD_GET_VERSION     0x16
#define KFSID_PARAM_CMD_END             0x1F
#define KFSID_PARAM_STATUS_OK           0x00

#define PARAM_MAX_COUNT                 12
#define PARAM_NAME_LENGTH               31

typedef struct
{
    char name[PARAM_NAME_LENGTH + 1];
    uint8_t type;
    uint32_t minimum;
    uint32_t maximum;
    uint32_t step;
    uint32_t value;
} ParamEntry;

static ParamEntry params[PARAM_MAX_COUNT];
static uint8_t paramCount;

char g_kfsidDeviceState[16] = "Unchecked";
char g_kfsidInstalledVersion[17] = "--";

static void protocolWrite(uint8_t value)
{
    POKE(KFSID_PROTOCOL_REG, value);
}

static uint8_t protocolRead(void)
{
    return PEEK(KFSID_PROTOCOL_REG);
}

static uint32_t protocolRead32(void)
{
    uint32_t value;

    value = protocolRead();
    value |= (uint32_t)protocolRead() << 8;
    value |= (uint32_t)protocolRead() << 16;
    value |= (uint32_t)protocolRead() << 24;
    return value;
}

static void protocolWrite32(uint32_t value)
{
    protocolWrite((uint8_t)value);
    protocolWrite((uint8_t)(value >> 8));
    protocolWrite((uint8_t)(value >> 16));
    protocolWrite((uint8_t)(value >> 24));
}

static uint8_t paramSessionStart(void)
{
    protocolWrite(KFSID_PARAM_START_MAGIC);
    return protocolRead() == KFSID_PARAM_START_ACK;
}

static void paramSessionEnd(void)
{
    protocolWrite(KFSID_PARAM_CMD_END);
    (void)(protocolRead() == KFSID_PARAM_END_ACK);
}

static uint8_t paramLoadVersion(void)
{
    uint8_t i;
    uint8_t length;

    protocolWrite(KFSID_PARAM_CMD_GET_VERSION);
    length = protocolRead();
    if (length == 0 || length >= sizeof(g_kfsidInstalledVersion))
        return 0;

    for (i = 0; i < length; ++i)
        g_kfsidInstalledVersion[i] = (char)protocolRead();
    g_kfsidInstalledVersion[length] = '\0';
    return 1;
}

static uint8_t paramLoadInfo(uint8_t index, ParamEntry* entry)
{
    uint8_t i;
    uint8_t nameLength;

    protocolWrite(KFSID_PARAM_CMD_GET_INFO);
    protocolWrite(index);

    entry->type = protocolRead();
    entry->minimum = protocolRead32();
    entry->maximum = protocolRead32();
    entry->step = protocolRead32();
    nameLength = protocolRead();

    if (nameLength > PARAM_NAME_LENGTH)
        return 0;

    for (i = 0; i < nameLength; ++i)
        entry->name[i] = (char)protocolRead();
    entry->name[nameLength] = '\0';

    if (entry->step == 0)
        entry->step = 1;
    return 1;
}

static uint8_t paramLoadValue(uint8_t index, ParamEntry* entry)
{
    protocolWrite(KFSID_PARAM_CMD_GET_VALUE);
    protocolWrite(index);
    entry->value = protocolRead32();
    return 1;
}

static uint8_t paramSetValue(uint8_t index, uint32_t value)
{
    protocolWrite(KFSID_PARAM_CMD_SET_VALUE);
    protocolWrite(index);
    protocolWrite32(value);
    return protocolRead() == KFSID_PARAM_STATUS_OK;
}

static uint8_t paramLoadTable(void)
{
    uint8_t i;

    protocolWrite(KFSID_PARAM_CMD_GET_COUNT);
    paramCount = protocolRead();
    if (paramCount == 0 || paramCount > PARAM_MAX_COUNT)
        return 0;

    for (i = 0; i < paramCount; ++i)
    {
        if (!paramLoadInfo(i, &params[i]) || !paramLoadValue(i, &params[i]))
            return 0;
    }
    return 1;
}

uint8_t refreshDeviceInfo(void)
{
    if (!paramSessionStart())
    {
        strcpy(g_kfsidDeviceState, "Not detected");
        strcpy(g_kfsidInstalledVersion, "--");
        return 0;
    }

    if (!paramLoadVersion())
    {
        /* The first parameter-protocol firmware did not expose a version.
         * End the session so its updater protocol remains available. */
        paramSessionEnd();
        strcpy(g_kfsidDeviceState, "Legacy firmware");
        strcpy(g_kfsidInstalledVersion, "Unknown");
        return 1;
    }

    paramSessionEnd();
    strcpy(g_kfsidDeviceState, "Ready");
    return 1;
}

void checkDeviceInfo(void)
{
    const char* lines[4];
    char versionLine[32];

    (void)refreshDeviceInfo();
    strcpy(versionLine, "Firmware: ");
    strcat(versionLine, g_kfsidInstalledVersion);
    lines[0] = "KungFuSID status";
    lines[1] = g_kfsidDeviceState;
    lines[2] = versionLine;
    lines[3] = NULL;
    screenPrintSimpleDialog(lines);
}

void autoTestDevice(void)
{
    uint8_t i;
    uint8_t passed = 1;
    const char* lines[5];
    char countLine[24];

    if (!paramSessionStart())
    {
        strcpy(g_kfsidDeviceState, "Not detected");
        passed = 0;
    }
    else if (!paramLoadVersion() || !paramLoadTable())
    {
        paramSessionEnd();
        strcpy(g_kfsidDeviceState, "Protocol error");
        passed = 0;
    }
    else
    {
        for (i = 0; i < paramCount; ++i)
        {
            if (params[i].value < params[i].minimum ||
                params[i].value > params[i].maximum || params[i].step == 0)
            {
                passed = 0;
                break;
            }
        }
        paramSessionEnd();
        strcpy(g_kfsidDeviceState, passed ? "Ready" : "Parameter error");
    }

    sprintf(countLine, "Parameters checked: %u", passed ? paramCount : 0);
    lines[0] = passed ? "Auto test passed" : "Auto test failed";
    lines[1] = g_kfsidDeviceState;
    lines[2] = countLine;
    lines[3] = "No flash data was changed.";
    lines[4] = NULL;
    screenPrintSimpleDialog(lines);
}

static void paramMessage(const char* message)
{
    gotoxy(0, 23);
    revers(0);
    textcolor(COLOR_YELLOW);
    cclear(40);
    gotoxy(0, 23);
    cputs(message);
}

static void paramDraw(uint8_t selected)
{
    uint8_t i;

    clrscr();
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_GREEN);
    cputsxy(0, 0, "KungFuSID parameter editor");
    textcolor(COLOR_GRAY2);
    cputsxy(0, 1, "Cursor: select/change  S:save D:defaults");
    cputsxy(0, 2, "STOP/Q: exit");

    for (i = 0; i < paramCount; ++i)
    {
        gotoxy(0, (uint8_t)(4 + i));
        revers(i == selected);
        cprintf(" %-25s %10lu ", params[i].name,
                (unsigned long)params[i].value);
        revers(0);
    }
}

static void paramChange(uint8_t selected, int8_t direction)
{
    ParamEntry* entry = &params[selected];
    uint32_t value = entry->value;

    if (direction < 0)
    {
        if (value <= entry->minimum || value - entry->minimum < entry->step)
            value = entry->minimum;
        else
            value -= entry->step;
    }
    else
    {
        if (value >= entry->maximum || entry->maximum - value < entry->step)
            value = entry->maximum;
        else
            value += entry->step;
    }

    if (value != entry->value && paramSetValue(selected, value))
        entry->value = value;
}

static void paramWaitForFlash(void)
{
    uint8_t i;
    for (i = 0; i < 180; ++i)
        waitvsync();
}

void editParameters(void)
{
    uint8_t selected = 0;
    uint8_t active = 1;
    char key;

    if (!paramSessionStart())
    {
        paramMessage("KungFuSID parameter protocol unavailable");
        (void)cgetc();
        return;
    }

    if (!paramLoadTable())
    {
        paramSessionEnd();
        paramMessage("Could not load parameter table");
        (void)cgetc();
        return;
    }

    paramDraw(selected);
    while (active)
    {
        key = cgetc();
        switch (key)
        {
        case CH_CURS_UP:
            selected = selected ? selected - 1 : paramCount - 1;
            paramDraw(selected);
            break;
        case CH_CURS_DOWN:
            selected = (uint8_t)((selected + 1) % paramCount);
            paramDraw(selected);
            break;
        case CH_CURS_LEFT:
            paramChange(selected, -1);
            paramDraw(selected);
            break;
        case CH_CURS_RIGHT:
            paramChange(selected, 1);
            paramDraw(selected);
            break;
        case 's':
        case 'S':
            protocolWrite(KFSID_PARAM_CMD_SAVE);
            paramMessage("Saving parameters...");
            paramWaitForFlash();
            if (protocolRead() == KFSID_PARAM_STATUS_OK)
                paramMessage("Saved; backend changes after restart");
            else
                paramMessage("Parameter save failed");
            break;
        case 'd':
        case 'D':
            protocolWrite(KFSID_PARAM_CMD_LOAD_DEFAULT);
            if (protocolRead() == KFSID_PARAM_STATUS_OK)
            {
                (void)paramLoadTable();
                paramDraw(selected);
                paramMessage("Defaults loaded; press S to save");
            }
            break;
        case CH_STOP:
        case 'q':
        case 'Q':
            active = 0;
            break;
        }
    }

    paramSessionEnd();
}
