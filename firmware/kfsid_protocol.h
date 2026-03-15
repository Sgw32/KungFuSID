#ifndef KFSID_PROTOCOL_H
#define KFSID_PROTOCOL_H

#include <stdbool.h>

#include "common.h"

#define KFSID_PROTOCOL_REGISTER 29

#define FW_UPDATE_START_MAGIC  0xA5
#define FW_UPDATE_START_ACK    0x5A
#define FW_UPDATE_END_ACK      0xE5

#define KFSID_PARAM_START_MAGIC 0xC1
#define KFSID_PARAM_START_ACK   0x1C
#define KFSID_PARAM_END_ACK     0xE1

void kfsid_protocol_init(void);
bool kfsid_protocol_audio_enabled(void);
void kfsid_protocol_write(u8 value);
u8 kfsid_protocol_peek(void);
void kfsid_protocol_consume(void);

#endif
