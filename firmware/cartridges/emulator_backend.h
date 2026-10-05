#ifndef EMULATOR_BACKEND_H
#define EMULATOR_BACKEND_H

#include <stdint.h>

typedef void (*emulator_cycle_handler_t)(void);
typedef void (*emulator_write_handler_t)(uint8_t address, uint8_t value);

extern emulator_cycle_handler_t emulator_backend_cycle_handler;

void emulator_backend_init(void);
void emulator_backend_reset(void);
void emulator_backend_write(uint8_t address, uint8_t value);

#endif
