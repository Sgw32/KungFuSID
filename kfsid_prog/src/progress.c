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
#include <conio.h>
#include <stdint.h>

#include "progress.h"
#include "screen.h"

#define PROGRESS_WIDTH 30

static uint8_t progressCells;

static void progressDrawBar(void)
{
    uint8_t i;

    gotoxy(5, 17);
    textcolor(COLOR_EXTRA);
    for (i = 0; i < PROGRESS_WIDTH; ++i)
        cputc(i < progressCells ? 0xA0 : '.');
    textcolor(COLOR_FOREGROUND);
}

void progressInit(void)
{
    progressCells = 0;
}

void progressShow(void)
{
    textcolor(COLOR_LIGHTFRAME);
    screenPrintBox(3, 14, 34, 6);
    textcolor(COLOR_FOREGROUND);
    cputsxy(11, 15, "Flashing progress");
    progressDrawBar();
}

void __fastcall__ progressSet(uint16_t completed, uint16_t total)
{
    uint8_t cells;

    if (total == 0)
        cells = 0;
    else if (completed >= total)
        cells = PROGRESS_WIDTH;
    else
        cells = (uint8_t)(((unsigned long)completed * PROGRESS_WIDTH) / total);

    if (cells != progressCells)
    {
        progressCells = cells;
        progressDrawBar();
    }
}
