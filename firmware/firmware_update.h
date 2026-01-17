#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include <stdbool.h>
#include "common.h"

#define FW_UPDATE_REGISTER     29
#define FW_UPDATE_START_MAGIC  0xA5
#define FW_UPDATE_START_ACK    0x5A
#define FW_UPDATE_END_ACK      0xE5

void firmware_update_init(void);
bool firmware_update_active(void);
bool firmware_update_sound_enabled(void);
void firmware_update_write(u8 value);
bool firmware_update_read(u8 *value);

#endif
