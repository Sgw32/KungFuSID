#ifndef FLASH_H
#define FLASH_H

#include <stddef.h>
#include "common.h"

void flash_sector_program(s8 sector, void *dest, void *src, size_t bytes);

#endif
