#ifndef AY3_BACKEND_H
#define AY3_BACKEND_H

#include <stdint.h>

void ay3_backend_reset(void);
void ay3_backend_write(uint8_t address, uint8_t value);
void ay3_backend_cycle(void);

#endif
