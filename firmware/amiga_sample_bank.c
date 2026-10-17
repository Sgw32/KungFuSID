#include "amiga_sample_bank.h"

#include <stddef.h>
#include <string.h>

#include "flash.h"

#define BANK_A_BASE ((const uint8_t*)0x08020000UL)
#define BANK_B_BASE ((const uint8_t*)0x08080000UL)

static const uint8_t* active_bank;
static uint8_t* upload_bank;
static uint32_t upload_size;
static uint32_t upload_generation;
static uint8_t upload_work[256] __attribute__((aligned(4)));

static uint32_t bank_crc32(const volatile uint8_t* data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;
    uint8_t bit;
    for (i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((0U - (crc & 1U)) & 0xEDB88320UL);
    }
    return crc ^ 0xFFFFFFFFUL;
}

static bool bank_valid(const uint8_t* base, bool require_commit,
                       uint32_t expected_size)
{
    const amiga_bank_header_t* h = (const amiga_bank_header_t*)base;
    uint16_t i;

    if (memcmp(h->magic, "KFSAMP1\0", 8) != 0 || h->version != 1U ||
        h->header_size != sizeof(*h) || h->entry_size != sizeof(amiga_bank_entry_t) ||
        h->instrument_count == 0U || h->instrument_count > AMIGA_BANK_MAX_INSTRUMENTS ||
        h->total_bytes < h->pcm_offset || h->total_bytes > AMIGA_BANK_MAX_BYTES ||
        h->directory_offset < sizeof(*h) ||
        h->directory_offset > h->pcm_offset ||
        h->pcm_offset - h->directory_offset <
            (uint32_t)h->entry_size * AMIGA_BANK_MAX_INSTRUMENTS ||
        (expected_size && h->total_bytes != expected_size) ||
        (require_commit && h->commit != AMIGA_BANK_COMMIT))
        return false;

    if (bank_crc32(base + h->directory_offset,
                   (uint32_t)h->entry_size * AMIGA_BANK_MAX_INSTRUMENTS) != h->directory_crc32 ||
        bank_crc32(base + h->pcm_offset, h->total_bytes - h->pcm_offset) != h->pcm_crc32)
        return false;

    for (i = 0; i < h->instrument_count; ++i)
    {
        const amiga_bank_entry_t* e = (const amiga_bank_entry_t*)
            (base + h->directory_offset + (uint32_t)i * h->entry_size);
        if (e->length == 0U || e->pcm_offset < h->pcm_offset ||
            e->pcm_offset > h->total_bytes ||
            e->length > h->total_bytes - e->pcm_offset ||
            ((e->flags & AMIGA_SAMPLE_LOOP) &&
             (e->loop_length == 0U || e->loop_start > e->length ||
              e->loop_length > e->length - e->loop_start)))
            return false;
    }
    return true;
}

void amiga_sample_bank_init(void)
{
    bool a = bank_valid(BANK_A_BASE, true, 0);
    bool b = bank_valid(BANK_B_BASE, true, 0);
    if (a && b)
    {
        const amiga_bank_header_t* ah = (const amiga_bank_header_t*)BANK_A_BASE;
        const amiga_bank_header_t* bh = (const amiga_bank_header_t*)BANK_B_BASE;
        active_bank = ((int32_t)(bh->generation - ah->generation) > 0) ? BANK_B_BASE : BANK_A_BASE;
    }
    else
        active_bank = a ? BANK_A_BASE : (b ? BANK_B_BASE : NULL);
}

bool amiga_sample_bank_available(void) { return active_bank != NULL; }

const amiga_bank_header_t* amiga_sample_bank_header(void)
{
    return (const amiga_bank_header_t*)active_bank;
}

const amiga_bank_entry_t* amiga_sample_bank_entry(uint8_t instrument_id)
{
    const amiga_bank_header_t* h;
    if (!active_bank || instrument_id == 0U)
        return NULL;
    h = (const amiga_bank_header_t*)active_bank;
    if (instrument_id > h->instrument_count)
        return NULL;
    return (const amiga_bank_entry_t*)(active_bank + h->directory_offset +
                                       (uint32_t)(instrument_id - 1U) * h->entry_size);
}

const int8_t* amiga_sample_bank_pcm(const amiga_bank_entry_t* entry)
{
    return active_bank && entry ? (const int8_t*)(active_bank + entry->pcm_offset) : NULL;
}

bool amiga_sample_bank_upload_begin(uint32_t bytes)
{
    uint8_t first_sector;
    uint8_t i;
    const amiga_bank_header_t* active = (const amiga_bank_header_t*)active_bank;
    if (bytes < sizeof(amiga_bank_header_t) || bytes > AMIGA_BANK_MAX_BYTES)
        return false;
    upload_bank = (uint8_t*)((active_bank == BANK_B_BASE) ? BANK_A_BASE : BANK_B_BASE);
    first_sector = (upload_bank == (uint8_t*)BANK_A_BASE) ? 5U : 8U;
    upload_size = bytes;
    upload_generation = active ? active->generation + 1U : 1U;
    for (i = 0; i < 3U; ++i)
        if (!flash_sector_erase_only((uint8_t)(first_sector + i)))
            return false;
    return true;
}

bool amiga_sample_bank_upload_write(uint32_t offset, const uint8_t* data,
                                    uint32_t bytes)
{
    uint32_t allowed;
    if (!upload_bank || (offset & 3U) || (bytes & 3U) || offset >= upload_size)
        return false;
    allowed = upload_size - offset;
    if (bytes > allowed)
        bytes = (allowed + 3U) & ~3U;
    if (offset == 0U)
    {
        if (bytes > sizeof(upload_work))
            return false;
        memcpy(upload_work, data, bytes);
        ((amiga_bank_header_t*)upload_work)->generation = upload_generation;
        ((amiga_bank_header_t*)upload_work)->commit = 0xFFFFFFFFUL;
        data = upload_work;
    }
    return flash_program_aligned(upload_bank + offset, data, bytes);
}

bool amiga_sample_bank_upload_finish(void)
{
    uint32_t commit = AMIGA_BANK_COMMIT;
    if (!upload_bank || !bank_valid(upload_bank, false, upload_size) ||
        !flash_program_aligned(upload_bank + offsetof(amiga_bank_header_t, commit),
                               &commit, sizeof(commit)))
        return false;
    flash_cache_reset();
    if (!bank_valid(upload_bank, true, upload_size))
        return false;
    active_bank = upload_bank;
    upload_bank = NULL;
    return true;
}
