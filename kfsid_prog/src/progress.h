/* KungFuSID adaptation of EasyProg, (c) 2009-2012 Thomas Giesel.
 * Distributed under the zlib license; this notice must not be removed. */
#ifndef PROGRESS_H_
#define PROGRESS_H_

#include <stdint.h>

void progressInit(void);
void progressShow(void);
void __fastcall__ progressSet(uint16_t completed, uint16_t total);

#endif
