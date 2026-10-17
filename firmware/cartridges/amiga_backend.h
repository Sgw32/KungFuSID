#ifndef AMIGA_BACKEND_H
#define AMIGA_BACKEND_H

#include <stdint.h>

void amiga_backend_reset(void);
void amiga_backend_write(uint8_t address, uint8_t value);
void amiga_backend_cycle(void);

#endif
