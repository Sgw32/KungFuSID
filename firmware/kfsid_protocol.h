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

#define KFSID_PARAM_CMD_GET_COUNT    0x10
#define KFSID_PARAM_CMD_GET_VALUE    0x11
#define KFSID_PARAM_CMD_SET_VALUE    0x12
#define KFSID_PARAM_CMD_SAVE         0x13
#define KFSID_PARAM_CMD_LOAD_DEFAULT 0x14
#define KFSID_PARAM_CMD_GET_INFO     0x15
#define KFSID_PARAM_CMD_END          0x1F

#define KFSID_PARAM_STATUS_OK        0x00
#define KFSID_PARAM_STATUS_ERROR     0xFF

void kfsid_protocol_init(void);
bool kfsid_protocol_audio_enabled(void);
bool kfsid_protocol_restart_pending(void);
void kfsid_protocol_write(u8 value);
u8 kfsid_protocol_peek(void);
void kfsid_protocol_consume(void);

#endif
