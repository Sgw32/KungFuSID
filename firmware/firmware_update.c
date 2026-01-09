#include "firmware_update.h"
#include "memory.h"
#include "flash.h"
#include "stm32f4xx.h"

#define FW_UPDATE_SECTOR_COUNT 4
#define FW_UPDATE_SECTOR_SIZE  (16 * 1024)

typedef enum
{
    FW_STATE_IDLE = 0,
    FW_STATE_WAIT_START_READ,
    FW_STATE_WAIT_INDEX,
    FW_STATE_WAIT_INDEX_READ,
    FW_STATE_WRITE_DATA,
    FW_STATE_WAIT_CHECKSUM_READ,
    FW_STATE_WAIT_END_READ,
} firmware_update_state_t;

static const u8 fw_end_sequence[] = "KFSID_END";
static const u8 fw_end_sequence_length = sizeof(fw_end_sequence) - 1;

static volatile bool fw_update_active = false;
static firmware_update_state_t fw_update_state = FW_STATE_IDLE;
static u8 fw_pending_read = 0;
static bool fw_pending_valid = false;
static firmware_update_state_t fw_next_state = FW_STATE_IDLE;

static u8 fw_current_sector = 0;
static u32 fw_buffer_offset = 0;
static u32 fw_buffer_count = 0;
static u8 fw_buffer_checksum = 0;
static u8 fw_end_sequence_index = 0;

static void fw_set_pending_read(u8 value, firmware_update_state_t next_state)
{
    fw_pending_read = value;
    fw_pending_valid = true;
    fw_next_state = next_state;
}

static void fw_reset_buffer_state(void)
{
    fw_buffer_count = 0;
    fw_buffer_checksum = 0;
}

static void fw_start_update(void)
{
    fw_update_active = true;
    fw_update_state = FW_STATE_WAIT_START_READ;
    fw_end_sequence_index = 0;
    fw_set_pending_read(FW_UPDATE_START_ACK, FW_STATE_WAIT_INDEX);
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

void firmware_update_init(void)
{
    fw_update_active = false;
    fw_update_state = FW_STATE_IDLE;
    fw_pending_valid = false;
    fw_end_sequence_index = 0;
    fw_reset_buffer_state();
}

bool firmware_update_active(void)
{
    return fw_update_active;
}

bool firmware_update_sound_enabled(void)
{
    return !fw_update_active;
}

void firmware_update_write(u8 value)
{
    if (!fw_update_active)
    {
        if (value == FW_UPDATE_START_MAGIC)
        {
            fw_start_update();
        }
        return;
    }

    if (fw_update_state == FW_STATE_WAIT_END_READ)
    {
        return;
    }

    if (fw_update_state == FW_STATE_WAIT_INDEX)
    {
        if (fw_match_end_sequence(value))
        {
            fw_update_state = FW_STATE_WAIT_END_READ;
            fw_set_pending_read(FW_UPDATE_END_ACK, FW_STATE_WAIT_END_READ);
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
        fw_set_pending_read(value, FW_STATE_WRITE_DATA);
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
            fw_set_pending_read(fw_buffer_checksum, FW_STATE_WAIT_INDEX);
        }
    }
}

bool firmware_update_read(u8 *value)
{
    if (!fw_update_active)
    {
        return false;
    }

    if (fw_pending_valid)
    {
        *value = fw_pending_read;
        fw_pending_valid = false;
        fw_update_state = fw_next_state;
        if (fw_update_state == FW_STATE_WAIT_END_READ)
        {
            fw_update_active = false;
            fw_update_state = FW_STATE_IDLE;
        }
        return true;
    }

    *value = 0;
    return true;
}
