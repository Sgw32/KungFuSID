/*
 * Copyright (c) 2019-2025 Kim Jørgensen
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
 ******************************************************************************
 * Derived from Draco Browser V1.0C 8 Bit (27.12.2009)
 * Created 2009 by Sascha Bader
 *
 * The code can be used freely as long as you retain
 * a notice describing original source and author.
 *
 * THE PROGRAMS ARE DISTRIBUTED IN THE HOPE THAT THEY WILL BE USEFUL,
 * BUT WITHOUT ANY WARRANTY. USE THEM AT YOUR OWN RISK!
 *
 * Newer versions might be available here: http://www.sascha-bader.de/html/code.html
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <conio.h>
#include <cbm.h>
#include <errno.h>
#include <joystick.h>
#include "screen.h"
#include "base.h"
#include "kff_data.h"
#include "disk.h"
#include "launcher_asm.h"

/* declarations */
static void mainLoopEF3(void);
static void mainLoopKFF(void);
static void textReaderLoop(void);
static uint8_t menuLoop(void);

static void updateScreen(void);
static void help(void);
static bool isSearchChar(uint8_t c);
static uint8_t search(uint8_t c);
static void showDir(void);
static void updateDir(uint8_t last_selected);
static void printDirPage(void);
static void printElement(uint8_t pos);

static void showKFFMessage(uint8_t color);
static void showMessage(const char *text, uint8_t color);

/* definitions */
#define EF3_USB_CMD_LEN 12

static bool isC128 = false;

static char linebuffer[SCREENW+1];
static uint8_t searchLen;
static uint8_t *bigBuffer = NULL;
static uint16_t *pageBuffer = NULL;

// Use different name to bypass FW update check for NTSC users (prior v1.32)
#define KUNG_FU_FLASH_VER_UPD "Kung Fu SID "
#define KUNG_FU_FLASH_VER "Kung Fu SID V1.0C"

static const char programBar[] = {"          " KUNG_FU_FLASH_VER "           "};
static const char menuBar[] =    {"          " KUNG_FU_FLASH_VER "  <F1> Help"};

int main(void)
{
    initScreen(COLOR_BLACK, COLOR_BLACK, TEXTC);
    clrscr();
    textcolor(TEXTC);
    updateScreen();
    cputsxy(4, 4, "KUNGFUSID UPDATER");
    cputsxy(4, 6, "C64 PRG BUILD READY");
    cputsxy(4, 8, "INSERT UPDATER MEDIA");
    cputsxy(4, 10, "");
    if (joy_install(&joy_static_stddrv) != JOY_ERR_OK)
    {
        showMessage("Failed to install joystick driver.", ERRORC);
        while (true);
    }

    /*
     * KungFuFlash updater logic disabled for PRG builds.
     *
     * The original KFF/EF3 communication loops are intentionally
     * commented out to keep this updater as a simple PRG program.
     */
    /*
    uint8_t i, tmp, tst;
    dir = (Directory *)malloc(sizeof(Directory));
    bigBuffer = (uint8_t *)dir;
    pageBuffer = (uint16_t *)dir;

    if (dir == NULL)
    {
        showMessage("Failed to allocate memory.", ERRORC);
        while (true);
    }

    

    isC128 = is_c128() != 0;

    if (USB_STATUS == KFF_ID_VALUE) // KFF mode
    {
        mainLoopKFF();
    }
    else    // EF3 mode
    {
        // We can also end up here if there is a hardware problem
        // Test EF3 RAM to make sure we are indeed in EF3 mode
        tst = tmp = EF_RAM_TEST;
        for (i=0; i<=8; i++)
        {
            if (EF_RAM_TEST != tst)
            {
                showMessage("Communication with cartridge failed.", ERRORC);
                cprintf("Check hardware");
                while (true);
            }
            EF_RAM_TEST = tst = 1<<i;
        }
        EF_RAM_TEST = tmp;

        mainLoopEF3();
    }

    free(dir);
    */

    waitKey();
    return 0;
}

static void showKFFMessage(uint8_t color)
{
    // const char *text = kff_read_text();
    // showMessage(text, color);
}

static void showTextPage(uint16_t page)
{
    uint8_t chr, last_c = 0;

    clrscr();
    revers(1);
    textcolor(BACKC);
    cputsxy(0, 0, " >");
    KFF_READ_PTR = 0;
    
    cputs("< ");
    cputsxy(0, BOTTOM, programBar);
    revers(0);
    textcolor(TEXTC);
    gotoxy(0, 1);

    KFF_READ_PTR = pageBuffer[page];
    while (true)
    {
        chr = KFF_DATA;
        if (!chr || wherey() >= 24)
        {
            (KFF_READ_PTR)--;
            break;
        }

        if (chr == 0x0a && last_c != 0x0d)  // Handle line endings
        {
            cputs("\r\n");
        }
        else if (chr == 0x09)   // Handle tab
        {
            if ((wherex() % 2) == 0)
            {
                cputs("  ");
            }
            else
            {
                cputc(' ');
            }
        }
        else
        {
            cputc(chr);
        }

        last_c = chr;
    }

    pageBuffer[page + 1] = KFF_READ_PTR;
}

static void textReaderLoop(void)
{
    
}

static void updateScreen(void)
{
    clrscr();
    revers(0);
    textcolor(BACKC);
    drawFrame();
    revers(1);
    cputsxy(0, BOTTOM, menuBar);
    revers(0);

    showDir();
}

static uint8_t menuLoop(void)
{
    return 0;
}

static bool isSearchChar(uint8_t c)
{
    return (c >= 0x20 && c <= 0x5f) || (c >= 0xc1 && c <= 0xda);
}

static uint8_t search(uint8_t c)
{
    return 0;
}

static void showDir(void)
{
    textcolor(BACKC);
    revers(1);
    cputsxy(1, 0, linebuffer);

    revers(0);
}

static void help(void)
{
    clrscr();
    revers(0);
    textcolor(TEXTC);
    cputsxy(0, 0, programBar);
    textcolor(BACKC);
    cputsxy(10, 1, "\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3\xa3");

    cputsxy(2, 2, "github.com/KimJorgensen/KungFuFlash");

    textcolor(TEXTC);
    cputsxy(0, 5, "<CRSR> or Joy       Change selection");
    cputsxy(0, 6, "<RETURN> or Fire    Run/Change Dir");
    cputsxy(0, 7, " + <SHIFT> or Hold  Options");
    cputsxy(0, 8, "<HOME>              Root Dir");
    cputsxy(0, 9, "<DEL> or Fire left  Dir Up");
    cputsxy(0, 10, "<A-Z> or Fire up    Search");
    cputsxy(0, 11, "<F1> or Fire down   Help");
    cputsxy(0, 12, "<F5> or Fire right  Settings");
    cputsxy(0, 13, "<F6>                C128 Mode");
    cputsxy(0, 14, "<F7>                BASIC (Cart Active)");
    cputsxy(0, 15, "<F8>                Kill");
    cputsxy(0, 16, "<RUN/STOP>          Reset");

    cputsxy(0, 18, "Use joystick in port 2");

    textcolor(COLOR_RED);
    cputsxy(0, 20, "KUNG FU FLASH IS PROVIDED WITH NO");
    cputsxy(0, 21, "WARRANTY OF ANY KIND.");
    textcolor(COLOR_LIGHTRED);
    cputsxy(0, 22, "USE IT AT YOUR OWN RISK!");

    gotoxy(0, 24);
    waitKey();
    updateScreen();
    waitRelease();
}

static void updateDir(uint8_t last_selected)
{
    
}

static void printDirPage(void)
{
    
}

static void printElement(uint8_t element_no)
{
    char *element;
    textcolor(WARNC);
    revers(1);

    cputsxy(1, 1+element_no, element);
    revers(0);
}

static void showMessage(const char *text, uint8_t color)
{
    clrscr();

    revers(1);
    textcolor(BACKC);
    cputsxy(0, 0, "                                        ");
    cputsxy(0, BOTTOM, programBar);
    revers(0);

    textcolor(color);
    cputsxy(0, 3, text);

    cputs("\r\n\r\n\r\n");
}
