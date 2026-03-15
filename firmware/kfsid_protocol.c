#include "kfsid_protocol.h"

#include "memory.h"
#include "flash.h"
#include "stm32f4xx.h"
#include "xparam_eeprom.h"

#define FW_UPDATE_SECTOR_COUNT 4
#define FW_UPDATE_SECTOR_SIZE  (16 * 1024)

typedef enum
{
    KFSID_MODE_IDLE = 0,
    KFSID_MODE_FIRMWARE_UPDATE,
    KFSID_MODE_PARAM,
} kfsid_mode_t;

typedef enum
{
    FW_STATE_IDLE = 0,
    FW_STATE_WAIT_START_READ,
    FW_STATE_WAIT_INDEX,
    FW_STATE_WAIT_INDEX_READ,
    FW_STATE_WRITE_DATA,
    FW_STATE_WAIT_CHECKSUM_READ,
    FW_STATE_WAIT_END_READ,
} kfsid_fw_state_t;

typedef enum
{
    PARAM_STATE_WAIT_START_READ = 0,
    PARAM_STATE_WAIT_COMMAND,
    PARAM_STATE_WAIT_PARAM_INDEX_GET,
    PARAM_STATE_WAIT_PARAM_INDEX_SET,
    PARAM_STATE_WAIT_PARAM_VALUE0,
    PARAM_STATE_WAIT_PARAM_VALUE1,
    PARAM_STATE_WAIT_PARAM_VALUE2,
    PARAM_STATE_WAIT_PARAM_VALUE3,
    PARAM_STATE_STREAM_VALUE,
    PARAM_STATE_WAIT_END_READ,
} param_state_t;

static const u8 fw_end_sequence[] = "KFSID_END";
static const u8 fw_end_sequence_length = sizeof(fw_end_sequence) - 1;

#define PARAM_CMD_GET_COUNT    0x10
#define PARAM_CMD_GET_VALUE    0x11
#define PARAM_CMD_SET_VALUE    0x12
#define PARAM_CMD_SAVE         0x13
#define PARAM_CMD_LOAD_DEFAULT 0x14
#define PARAM_CMD_END          0x1F

#define PARAM_STATUS_OK        0x00
#define PARAM_STATUS_ERROR     0xFF

static kfsid_mode_t kfsid_mode = KFSID_MODE_IDLE;

static volatile bool protocol_busy = false;
static u8 protocol_pending_read = 255;
static bool protocol_pending_valid = false;

static kfsid_fw_state_t fw_update_state = FW_STATE_IDLE;
static kfsid_fw_state_t fw_next_state = FW_STATE_IDLE;
static u8 fw_current_sector = 0;
static u32 fw_buffer_offset = 0;
static u32 fw_buffer_count = 0;
static u8 fw_buffer_checksum = 0;
static u8 fw_end_sequence_index = 0;

static param_state_t param_state = PARAM_STATE_WAIT_START_READ;
static param_state_t param_next_state = PARAM_STATE_WAIT_START_READ;
static uint16_t param_index = 0;
static uint32_t param_value = 0;
static uint8_t param_stream_index = 0;

static void protocol_set_pending_read(u8 value, u8 next_state)
{
    protocol_pending_read = value;
    protocol_pending_valid = true;

    if (kfsid_mode == KFSID_MODE_FIRMWARE_UPDATE)
    {
        fw_next_state = (kfsid_fw_state_t)next_state;
    }
    else if (kfsid_mode == KFSID_MODE_PARAM)
    {
        param_next_state = (param_state_t)next_state;
    }
}

static void fw_reset_buffer_state(void)
{
    fw_buffer_count = 0;
    fw_buffer_checksum = 0;
}

static void fw_start_update(void)
{
    protocol_busy = true;
    kfsid_mode = KFSID_MODE_FIRMWARE_UPDATE;
    fw_update_state = FW_STATE_WAIT_START_READ;
    fw_end_sequence_index = 0;
    protocol_set_pending_read(FW_UPDATE_START_ACK, FW_STATE_WAIT_INDEX);
}

static void fw_program_sector(u8 sector)
{
    u32 offset = FW_UPDATE_SECTOR_SIZE * sector;
    flash_sector_program(sector, (u8 *)FLASH_BASE + offset,
                         dat_buffer + offset, FW_UPDATE_SECTOR_SIZE);
}

static bool fw_match_end_sequence(u8 value)
{
    if (value == fw_end_sequence[fw_end_sequence_index])
    {
        fw_end_sequence_index++;
        if (fw_end_sequence_index == fw_end_sequence_length)
        {
            return true;
        }
    }
    else
    {
        fw_end_sequence_index = (value == fw_end_sequence[0]) ? 1 : 0;
    }
    return false;
}

static void param_start_session(void)
{
    protocol_busy = true;
    kfsid_mode = KFSID_MODE_PARAM;
    param_state = PARAM_STATE_WAIT_START_READ;
    protocol_set_pending_read(KFSID_PARAM_START_ACK, PARAM_STATE_WAIT_COMMAND);
}

static void param_begin_stream(uint32_t value)
{
    param_value = value;
    param_stream_index = 0;
    protocol_set_pending_read((u8)(param_value & 0xFF), PARAM_STATE_STREAM_VALUE);
}

static void param_handle_command(u8 value)
{
    switch (value)
    {
    case PARAM_CMD_GET_COUNT:
        protocol_set_pending_read((u8)kfsid_params_count(), PARAM_STATE_WAIT_COMMAND);
        break;
    case PARAM_CMD_GET_VALUE:
        param_state = PARAM_STATE_WAIT_PARAM_INDEX_GET;
        break;
    case PARAM_CMD_SET_VALUE:
        param_state = PARAM_STATE_WAIT_PARAM_INDEX_SET;
        break;
    case PARAM_CMD_SAVE:
        protocol_set_pending_read(kfsid_params_save_to_flash() ? PARAM_STATUS_OK : PARAM_STATUS_ERROR,
                                  PARAM_STATE_WAIT_COMMAND);
        break;
    case PARAM_CMD_LOAD_DEFAULT:
        kfsid_params_load_defaults();
        kfsid_params_apply_runtime();
        protocol_set_pending_read(PARAM_STATUS_OK, PARAM_STATE_WAIT_COMMAND);
        break;
    case PARAM_CMD_END:
        param_state = PARAM_STATE_WAIT_END_READ;
        protocol_set_pending_read(KFSID_PARAM_END_ACK, PARAM_STATE_WAIT_END_READ);
        break;
    default:
        protocol_set_pending_read(PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
        break;
    }
}

void kfsid_protocol_init(void)
{
    protocol_busy = false;
    kfsid_mode = KFSID_MODE_IDLE;
    protocol_pending_valid = false;
    protocol_pending_read = 255;

    fw_update_state = FW_STATE_IDLE;
    fw_end_sequence_index = 0;
    fw_reset_buffer_state();

    param_state = PARAM_STATE_WAIT_START_READ;
    param_index = 0;
    param_value = 0;
    param_stream_index = 0;
}

bool kfsid_protocol_audio_enabled(void)
{
    return !protocol_busy;
}

void kfsid_protocol_write(u8 value)
{
    if (!protocol_busy)
    {
        if (value == FW_UPDATE_START_MAGIC)
        {
            fw_start_update();
        }
        else if (value == KFSID_PARAM_START_MAGIC)
        {
            param_start_session();
        }
        return;
    }

    if (kfsid_mode == KFSID_MODE_FIRMWARE_UPDATE)
    {
        if (fw_update_state == FW_STATE_WAIT_END_READ)
        {
            return;
        }

        if (fw_update_state == FW_STATE_WAIT_INDEX)
        {
            if (fw_match_end_sequence(value))
            {
                fw_update_state = FW_STATE_WAIT_END_READ;
                protocol_set_pending_read(FW_UPDATE_END_ACK, FW_STATE_WAIT_END_READ);
                return;
            }

            if (value >= FW_UPDATE_SECTOR_COUNT)
            {
                return;
            }

            fw_current_sector = value;
            fw_buffer_offset = FW_UPDATE_SECTOR_SIZE * fw_current_sector;
            fw_reset_buffer_state();
            fw_update_state = FW_STATE_WAIT_INDEX_READ;
            protocol_set_pending_read(value, FW_STATE_WRITE_DATA);
            return;
        }

        if (fw_update_state == FW_STATE_WRITE_DATA)
        {
            dat_buffer[fw_buffer_offset + fw_buffer_count] = value;
            fw_buffer_checksum ^= value;
            fw_buffer_count++;

            if (fw_buffer_count >= FW_UPDATE_SECTOR_SIZE)
            {
                fw_program_sector(fw_current_sector);
                fw_update_state = FW_STATE_WAIT_CHECKSUM_READ;
                protocol_set_pending_read(fw_buffer_checksum, FW_STATE_WAIT_INDEX);
            }
        }

        return;
    }

    if (kfsid_mode == KFSID_MODE_PARAM)
    {
        if (param_state == PARAM_STATE_WAIT_COMMAND)
        {
            param_handle_command(value);
            return;
        }

        if (param_state == PARAM_STATE_WAIT_PARAM_INDEX_GET)
        {
            uint32_t read_value = 0;
            if (kfsid_param_get_value(value, &read_value))
            {
                param_index = value;
                (void)param_index;
                param_begin_stream(read_value);
            }
            else
            {
                protocol_set_pending_read(PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
            }
            return;
        }

        if (param_state == PARAM_STATE_WAIT_PARAM_INDEX_SET)
        {
            if (value >= kfsid_params_count())
            {
                protocol_set_pending_read(PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
                return;
            }

            param_index = value;
            param_value = 0;
            param_state = PARAM_STATE_WAIT_PARAM_VALUE0;
            return;
        }

        if (param_state == PARAM_STATE_WAIT_PARAM_VALUE0)
        {
            param_value = (uint32_t)value;
            param_state = PARAM_STATE_WAIT_PARAM_VALUE1;
            return;
        }
        if (param_state == PARAM_STATE_WAIT_PARAM_VALUE1)
        {
            param_value |= ((uint32_t)value << 8);
            param_state = PARAM_STATE_WAIT_PARAM_VALUE2;
            return;
        }
        if (param_state == PARAM_STATE_WAIT_PARAM_VALUE2)
        {
            param_value |= ((uint32_t)value << 16);
            param_state = PARAM_STATE_WAIT_PARAM_VALUE3;
            return;
        }
        if (param_state == PARAM_STATE_WAIT_PARAM_VALUE3)
        {
            param_value |= ((uint32_t)value << 24);
            protocol_set_pending_read(kfsid_param_set_value(param_index, param_value) ? PARAM_STATUS_OK : PARAM_STATUS_ERROR,
                                      PARAM_STATE_WAIT_COMMAND);
            return;
        }
    }
}

u8 kfsid_protocol_peek(void)
{
    if (!protocol_busy)
    {
        return 255;
    }

    return protocol_pending_read;
}

void kfsid_protocol_consume(void)
{
    if (!protocol_busy)
    {
        return;
    }

    if (!protocol_pending_valid)
    {
        return;
    }

    protocol_pending_valid = false;

    if (kfsid_mode == KFSID_MODE_FIRMWARE_UPDATE)
    {
        fw_update_state = fw_next_state;
        if (fw_update_state == FW_STATE_WAIT_END_READ)
        {
            protocol_busy = false;
            fw_update_state = FW_STATE_IDLE;
            kfsid_mode = KFSID_MODE_IDLE;
        }
        return;
    }

    if (kfsid_mode == KFSID_MODE_PARAM)
    {
        if (param_state == PARAM_STATE_STREAM_VALUE)
        {
            param_stream_index++;
            if (param_stream_index < 4)
            {
                protocol_set_pending_read((u8)((param_value >> (8 * param_stream_index)) & 0xFF),
                                          PARAM_STATE_STREAM_VALUE);
            }
            else
            {
                param_state = PARAM_STATE_WAIT_COMMAND;
            }
            return;
        }

        param_state = param_next_state;

        if (param_state == PARAM_STATE_WAIT_END_READ)
        {
            protocol_busy = false;
            param_state = PARAM_STATE_WAIT_START_READ;
            kfsid_mode = KFSID_MODE_IDLE;
        }
    }
}
