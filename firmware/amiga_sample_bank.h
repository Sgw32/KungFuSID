#ifndef AMIGA_SAMPLE_BANK_H
#define AMIGA_SAMPLE_BANK_H

#include <stdbool.h>
#include <stdint.h>

#define AMIGA_BANK_MAX_BYTES       (384UL * 1024UL)
#define AMIGA_BANK_MAX_INSTRUMENTS 64U
#define AMIGA_BANK_COMMIT          0x4B53424BUL

#define AMIGA_SAMPLE_ONE_SHOT      0x0001U
#define AMIGA_SAMPLE_LOOP          0x0002U
#define AMIGA_SAMPLE_PERCUSSION    0x0004U

typedef struct __attribute__((packed))
{
    char magic[8];
    uint16_t version;
    uint16_t header_size;
    uint32_t generation;
    uint32_t total_bytes;
    uint16_t instrument_count;
    uint16_t entry_size;
    uint32_t directory_offset;
    uint32_t pcm_offset;
    uint32_t directory_crc32;
    uint32_t pcm_crc32;
    uint32_t flags;
    char bank_name[16];
    uint32_t commit;
} amiga_bank_header_t;

typedef struct __attribute__((packed))
{
    uint32_t pcm_offset;
    uint32_t length;
    uint32_t loop_start;
    uint32_t loop_length;
    uint32_t native_rate;
    uint32_t root_frequency_q16;
    uint16_t flags;
    uint8_t family;
    uint8_t variant;
    uint8_t volume;
    uint8_t brightness;
    uint8_t preferred_wave;
    uint8_t drum;
    char name[16];
    uint8_t centroid[16];
} amiga_bank_entry_t;

void amiga_sample_bank_init(void);
bool amiga_sample_bank_available(void);
const amiga_bank_header_t* amiga_sample_bank_header(void);
const amiga_bank_entry_t* amiga_sample_bank_entry(uint8_t instrument_id);
const int8_t* amiga_sample_bank_pcm(const amiga_bank_entry_t* entry);

bool amiga_sample_bank_upload_begin(uint32_t bytes);
bool amiga_sample_bank_upload_write(uint32_t offset, const uint8_t* data,
                                    uint32_t bytes);
bool amiga_sample_bank_upload_finish(void);

#endif
