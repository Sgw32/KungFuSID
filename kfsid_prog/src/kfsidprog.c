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
#include <cbm.h>
#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "filedlg.h"
#include "kfsidprog.h"
#include "params.h"
#include "progress.h"
#include "screen.h"
#include "texts.h"
#include "util.h"
#include "write.h"

static void showAbout(void);
static void showComingSoon(void);
static void toggleFastLoader(void);
static uint8_t returnTrue(void);
static void updateFastLoaderText(void);

uint8_t g_bFastLoaderEnabled;

/* Kept for loader compatibility; KungFuSID has no EasyFlash slots. */
uint8_t g_nSlots = 1;
uint8_t g_nSelectedSlot = 0;

static char strStatus[31];
static char strFastLoader[30];

extern ScreenMenu menuMain;
extern ScreenMenu menuOptions;
extern ScreenMenu menuExpert;
extern ScreenMenu menuHelp;

ScreenMenu menuMain =
{
    1, 2,
    0,
    &menuHelp,
    &menuOptions,
    {
        {
            "Check &status / version",
            checkDeviceInfo,
            returnTrue,
            0
        },
        {
            "&Update firmware from BIN",
            checkWriteUpdateBIN,
            returnTrue,
            0
        },
        {
            "Edit &parameters",
            editParameters,
            returnTrue,
            0
        },
        { NULL, NULL, 0, 0 }
    }
};

ScreenMenu menuOptions =
{
    7, 2,
    0,
    &menuMain,
    &menuExpert,
    {
        {
            strFastLoader,
            toggleFastLoader,
            returnTrue,
            SCREEN_MENU_ENTRY_FLAG_KEEP
        },
        { NULL, NULL, 0, 0 }
    }
};

ScreenMenu menuExpert =
{
    16, 2,
    0,
    &menuOptions,
    &menuHelp,
    {
        {
            "&Auto test",
            autoTestDevice,
            returnTrue,
            0
        },
        {
            "SID &Player",
            showComingSoon,
            returnTrue,
            0
        },
        { NULL, NULL, 0, 0 }
    }
};

ScreenMenu menuHelp =
{
    24, 2,
    0,
    &menuExpert,
    &menuMain,
    {
        {
            "&About",
            showAbout,
            returnTrue,
            0
        },
        { NULL, NULL, 0, 0 }
    }
};

static void refreshStatusLine(void)
{
    gotoxy(1, 23);
    cputs(strStatus);
    cclear((uint8_t)(38 - strlen(strStatus)));
}

void refreshMainScreen(void)
{
    screenPrintFrame();

    gotoxy(1, 1);
    textcolor(COLOR_EXTRA);
    cputc('M');
    textcolor(COLOR_FOREGROUND);
    cputs("enu  ");
    textcolor(COLOR_EXTRA);
    cputc('O');
    textcolor(COLOR_FOREGROUND);
    cputs("ptions  ");
    textcolor(COLOR_EXTRA);
    cputc('E');
    textcolor(COLOR_FOREGROUND);
    cputs("xpert  ");
    textcolor(COLOR_EXTRA);
    cputc('H');
    textcolor(COLOR_FOREGROUND);
    cputs("elp");

    textcolor(COLOR_LIGHTFRAME);
    screenPrintBox(3, 4, 34, 9);
    screenPrintSepLine(3, 36, 6);
    screenPrintSepLine(3, 36, 8);
    screenPrintSepLine(3, 36, 10);
    textcolor(COLOR_FOREGROUND);

    cputsxy(5, 5, "File:");
    cputsxy(16, 5, g_strFileName[0] ? g_strFileName : "--");
    cputsxy(5, 7, "File FW:");
    cputsxy(16, 7, firmwareFileVersion());
    cputsxy(5, 9, "Installed:");
    cputsxy(16, 9, g_kfsidInstalledVersion);
    cputsxy(5, 11, "State:");
    cputsxy(16, 11, g_kfsidDeviceState);

    progressShow();
    refreshStatusLine();
}

static uint8_t returnTrue(void)
{
    return 1;
}

static void showAbout(void)
{
    screenPrintSimpleDialog(apStrAbout);
}

static void showComingSoon(void)
{
    static const char* lines[] = { "SID Player", "Coming soon", NULL };
    screenPrintSimpleDialog(lines);
}

static void toggleFastLoader(void)
{
    g_bFastLoaderEnabled = !g_bFastLoaderEnabled;
    updateFastLoaderText();
}

void __fastcall__ setStatus(const char* pStrStatus)
{
    strncpy(strStatus, pStrStatus, sizeof(strStatus) - 1);
    strStatus[sizeof(strStatus) - 1] = '\0';
    refreshStatusLine();
}

static void __fastcall__ execMenu(ScreenMenu* pMenu)
{
    screenDoMenu(pMenu);
    refreshMainScreen();
}

static void updateFastLoaderText(void)
{
    strcpy(strFastLoader, "&Fastloader enabled: ");
    strcat(strFastLoader, g_bFastLoaderEnabled ? "Yes" : "No");
}

int main(void)
{
    char key;

    screenInit();
    progressInit();
    g_strFileName[0] = '\0';
    g_nDrive = *(uint8_t*)0xBA;
    if (g_nDrive < 8)
        g_nDrive = 8;

    g_bFastLoaderEnabled = 1;
    updateFastLoaderText();
    (void)refreshDeviceInfo();
    strcpy(strStatus, "Ready. Press <m> for Menu.");
    refreshMainScreen();
    showAbout();
    refreshMainScreen();

    for (;;)
    {
        if (!kbhit())
            continue;

        key = cgetc();
        switch (key)
        {
        case 'm':
            execMenu(&menuMain);
            break;
        case 'o':
            execMenu(&menuOptions);
            break;
        case 'e':
            execMenu(&menuExpert);
            break;
        case 'h':
            execMenu(&menuHelp);
            break;
        }
    }
    return 0;
}
