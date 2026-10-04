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
#include <stdio.h>

#include "texts.h"

const char* apStrAbout[] =
{
    "KungFuSID Programmer",
    "Version " EFVERSION,
    "Firmware update and setup tool",
    "based on EasyProg by skoe",
    "under the zlib license",
    "",
    "Compiled: " __DATE__ " " __TIME__,
    NULL
};

const char* apStrFileNoEasySplit[] =
{
    "This is not a valid split file",
    "or it is damaged.",
    NULL
};

const char* apStrAskErase[] =
{
    "This operation is not available",
    "in KungFuSID Programmer.",
    NULL
};

const char* apStrFlashWriteFailed[] =
{
    "Firmware update failed.",
    "KungFuSID was not changed safely.",
    NULL
};

const char* apStrFileOpenError[] =
{
    "Cannot open this file.",
    NULL
};

const char* apStrInvalidFirmware[] =
{
    "Firmware image is empty, damaged",
    "or larger than 64 KiB.",
    NULL
};

const char* apStrOutOfMemory[] =
{
    "Out of memory.",
    NULL
};

const char* apStrWriteComplete[] =
{
    "KungFuSID firmware updated.",
    "The device restarted successfully.",
    NULL
};

const char* apStrDirFull[] =
{
    "There are too many entries",
    "in this directory.",
    NULL
};

const char* apStrDifferentFile[] =
{
    "This part does not match",
    "the previous file parts.",
    NULL
};
