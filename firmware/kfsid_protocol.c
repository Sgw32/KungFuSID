#include "kfsid_protocol.h"

#include <string.h>

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
    PARAM_STATE_WAIT_PARAM_INDEX_INFO,
    PARAM_STATE_WAIT_PARAM_INDEX_SET,
    PARAM_STATE_WAIT_PARAM_VALUE0,
    PARAM_STATE_WAIT_PARAM_VALUE1,
    PARAM_STATE_WAIT_PARAM_VALUE2,
    PARAM_STATE_WAIT_PARAM_VALUE3,
    PARAM_STATE_STREAM_DATA,
    PARAM_STATE_WAIT_END_READ,
} param_state_t;

static const u8 fw_end_sequence[] = "KFSID_END";
static const u8 fw_end_sequence_length = sizeof(fw_end_sequence) - 1;

#define PARAM_STREAM_MAX       46U
#define PARAM_NAME_MAX         31U

static kfsid_mode_t kfsid_mode = KFSID_MODE_IDLE;

static volatile bool protocol_busy = false;
static volatile bool protocol_restart_pending = false;
static u8 protocol_pending_read = 255;
static bool protocol_pending_valid = false;

static kfsid_fw_state_t fw_update_state = FW_STATE_IDLE;
static kfsid_fw_state_t fw_next_state = FW_STATE_IDLE;
static u8 fw_current_sector = 0;
static u32 fw_buffer_offset = 0;
static u32 fw_buffer_count = 0;
static u8 fw_end_sequence_index = 0;

static param_state_t param_state = PARAM_STATE_WAIT_START_READ;
static param_state_t param_next_state = PARAM_STATE_WAIT_START_READ;
static uint16_t param_index = 0;
static uint32_t param_value = 0;
static u8 param_stream_data[PARAM_STREAM_MAX];
static u8 param_stream_length = 0;
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
}

static void fw_start_update(void)
{
    protocol_busy = true;
    kfsid_mode = KFSID_MODE_FIRMWARE_UPDATE;
    fw_update_state = FW_STATE_WAIT_START_READ;
    fw_end_sequence_index = 0;
    protocol_set_pending_read(FW_UPDATE_START_ACK, FW_STATE_WAIT_INDEX);
}

static u8 fw_program_sector(u8 sector)
{
    u32 offset = FW_UPDATE_SECTOR_SIZE * sector;
    const volatile u8* programmed_data =
        (const volatile u8*)FLASH_BASE + offset;
    u8 checksum = 0;

    flash_sector_program(sector, (u8 *)FLASH_BASE + offset,
                         dat_buffer + offset, FW_UPDATE_SECTOR_SIZE);

    /* Return a checksum of flash, not merely the received RAM buffer, so the
     * C64 can detect an erase/program failure before finalizing the update. */
    for (u32 i = 0; i < FW_UPDATE_SECTOR_SIZE; ++i)
    {
        checksum ^= programmed_data[i];
    }
    return checksum;
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

static void param_begin_stream(const u8* data, u8 length)
{
    memcpy(param_stream_data, data, length);
    param_stream_length = length;
    param_stream_index = 0;
    param_state = PARAM_STATE_STREAM_DATA;
    protocol_set_pending_read(param_stream_data[0], PARAM_STATE_STREAM_DATA);
}

static void param_begin_u32_stream(uint32_t value)
{
    u8 data[4];
    data[0] = (u8)value;
    data[1] = (u8)(value >> 8);
    data[2] = (u8)(value >> 16);
    data[3] = (u8)(value >> 24);
    param_begin_stream(data, sizeof(data));
}

static void param_begin_info_stream(u8 index)
{
    xparam_t* param = &kungfusid_params_table.params[index];
    size_t name_length = strlen(param->p_name);
    int32_t minimum = (int32_t)param->min;
    int32_t maximum = (int32_t)param->max;
    uint32_t step = (uint32_t)param->step_size;

    if (name_length > PARAM_NAME_MAX)
    {
        name_length = PARAM_NAME_MAX;
    }

    param_stream_data[0] = (u8)param->value_type;
    param_stream_data[1] = (u8)minimum;
    param_stream_data[2] = (u8)(minimum >> 8);
    param_stream_data[3] = (u8)(minimum >> 16);
    param_stream_data[4] = (u8)(minimum >> 24);
    param_stream_data[5] = (u8)maximum;
    param_stream_data[6] = (u8)(maximum >> 8);
    param_stream_data[7] = (u8)(maximum >> 16);
    param_stream_data[8] = (u8)(maximum >> 24);
    param_stream_data[9] = (u8)step;
    param_stream_data[10] = (u8)(step >> 8);
    param_stream_data[11] = (u8)(step >> 16);
    param_stream_data[12] = (u8)(step >> 24);
    param_stream_data[13] = (u8)name_length;
    memcpy(&param_stream_data[14], param->p_name, name_length);

    param_stream_length = (u8)(14U + name_length);
    param_stream_index = 0;
    param_state = PARAM_STATE_STREAM_DATA;
    protocol_set_pending_read(param_stream_data[0], PARAM_STATE_STREAM_DATA);
}

static void param_handle_command(u8 value)
{
    switch (value)
    {
    case KFSID_PARAM_CMD_GET_COUNT:
        protocol_set_pending_read((u8)kfsid_params_count(), PARAM_STATE_WAIT_COMMAND);
        break;
    case KFSID_PARAM_CMD_GET_VALUE:
        param_state = PARAM_STATE_WAIT_PARAM_INDEX_GET;
        break;
    case KFSID_PARAM_CMD_GET_INFO:
        param_state = PARAM_STATE_WAIT_PARAM_INDEX_INFO;
        break;
    case KFSID_PARAM_CMD_SET_VALUE:
        param_state = PARAM_STATE_WAIT_PARAM_INDEX_SET;
        break;
    case KFSID_PARAM_CMD_SAVE:
        protocol_set_pending_read(kfsid_params_save_to_flash() ? KFSID_PARAM_STATUS_OK : KFSID_PARAM_STATUS_ERROR,
                                  PARAM_STATE_WAIT_COMMAND);
        break;
    case KFSID_PARAM_CMD_LOAD_DEFAULT:
        kfsid_params_load_defaults();
        kfsid_params_apply_runtime();
        protocol_set_pending_read(KFSID_PARAM_STATUS_OK, PARAM_STATE_WAIT_COMMAND);
        break;
    case KFSID_PARAM_CMD_END:
        param_state = PARAM_STATE_WAIT_END_READ;
        protocol_set_pending_read(KFSID_PARAM_END_ACK, PARAM_STATE_WAIT_END_READ);
        break;
    default:
        protocol_set_pending_read(KFSID_PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
        break;
    }
}

void kfsid_protocol_init(void)
{
    protocol_busy = false;
    protocol_restart_pending = false;
    kfsid_mode = KFSID_MODE_IDLE;
    protocol_pending_valid = false;
    protocol_pending_read = 255;

    fw_update_state = FW_STATE_IDLE;
    fw_end_sequence_index = 0;
    fw_reset_buffer_state();

    param_state = PARAM_STATE_WAIT_START_READ;
    param_index = 0;
    param_value = 0;
    param_stream_length = 0;
    param_stream_index = 0;
}

bool kfsid_protocol_audio_enabled(void)
{
    return kfsid_mode != KFSID_MODE_FIRMWARE_UPDATE;
}

bool kfsid_protocol_restart_pending(void)
{
    return protocol_restart_pending;
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
            fw_buffer_count++;

            if (fw_buffer_count >= FW_UPDATE_SECTOR_SIZE)
            {
                u8 programmed_checksum = fw_program_sector(fw_current_sector);
                fw_update_state = FW_STATE_WAIT_CHECKSUM_READ;
                protocol_set_pending_read(programmed_checksum, FW_STATE_WAIT_INDEX);
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
                param_begin_u32_stream(read_value);
            }
            else
            {
                protocol_set_pending_read(KFSID_PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
            }
            return;
        }

        if (param_state == PARAM_STATE_WAIT_PARAM_INDEX_INFO)
        {
            if (value < kfsid_params_count())
            {
                param_begin_info_stream(value);
            }
            else
            {
                protocol_set_pending_read(KFSID_PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
            }
            return;
        }

        if (param_state == PARAM_STATE_WAIT_PARAM_INDEX_SET)
        {
            if (value >= kfsid_params_count())
            {
                protocol_set_pending_read(KFSID_PARAM_STATUS_ERROR, PARAM_STATE_WAIT_COMMAND);
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
            protocol_set_pending_read(kfsid_param_set_value(param_index, param_value) ? KFSID_PARAM_STATUS_OK : KFSID_PARAM_STATUS_ERROR,
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
            protocol_restart_pending = true;
            fw_update_state = FW_STATE_IDLE;
            kfsid_mode = KFSID_MODE_IDLE;
        }
        return;
    }

    if (kfsid_mode == KFSID_MODE_PARAM)
    {
        if (param_state == PARAM_STATE_STREAM_DATA)
        {
            param_stream_index++;
            if (param_stream_index < param_stream_length)
            {
                protocol_set_pending_read(param_stream_data[param_stream_index],
                                          PARAM_STATE_STREAM_DATA);
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
