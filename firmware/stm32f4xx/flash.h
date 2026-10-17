#ifndef FLASH_H
#define FLASH_H

#include <stddef.h>
#include <stdbool.h>
#include "common.h"

void flash_sector_program(s8 sector, void *dest, void *src, size_t bytes);
bool flash_sector_erase_only(u8 sector);
bool flash_program_aligned(void *dest, const void *src, size_t bytes);
void flash_cache_reset(void);

#endif
