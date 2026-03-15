#include "xparam_eeprom.h"

#include <string.h>

#include "flash.h"
#include "stm32f4xx.h"
#include "cartridges/sid.h"

#define KFSID_PARAM_FLASH_SECTOR   11U
#define KFSID_PARAM_FLASH_ADDRESS  (0x080E0000U)
#define KFSID_PARAM_FLASH_MAX_SIZE (128U * 1024U)

const kungfusid_parameters_t kungfusid_default_parameters = {
    PARAM_LIST(DEFINE_PARAM)
};

kungfusid_parameters_t kungfusid_parameters = {
    PARAM_LIST(DEFINE_PARAM)
};

xparam_table_t kungfusid_params_table = {
    .params = (xparam_t*)&kungfusid_parameters,
    .n_params = (sizeof(kungfusid_parameters_t) / sizeof(xparam_t)),
};

static size_t kfsid_param_blob_size(void)
{
    return XPARAM_IMAGE_SIZE(kungfusid_params_table.n_params);
}

void kfsid_params_load_defaults(void)
{
    memcpy(&kungfusid_parameters,
           &kungfusid_default_parameters,
           sizeof(kungfusid_parameters));
}

bool kfsid_params_load_from_flash(void)
{
    const uint8_t* flash_blob = (const uint8_t*)KFSID_PARAM_FLASH_ADDRESS;
    return xparam_table_from_blob(&kungfusid_params_table, (uint8_t*)flash_blob) != 0;
}

bool kfsid_params_save_to_flash(void)
{
    uint8_t* blob = xparam_table_to_blob(&kungfusid_params_table);
    if (!blob)
    {
        return false;
    }

    const size_t blob_size = kfsid_param_blob_size();
    const size_t aligned_size = (blob_size + 3U) & ~((size_t)3U);

    if (aligned_size > KFSID_PARAM_FLASH_MAX_SIZE)
    {
        free(blob);
        return false;
    }

    static uint8_t flash_write_buffer[KFSID_PARAM_FLASH_MAX_SIZE] __attribute__((aligned(4)));
    memset(flash_write_buffer, 0xFF, aligned_size);
    memcpy(flash_write_buffer, blob, blob_size);

    flash_sector_program((s8)KFSID_PARAM_FLASH_SECTOR,
                         (void*)KFSID_PARAM_FLASH_ADDRESS,
                         flash_write_buffer,
                         aligned_size);

    free(blob);
    return true;
}

void kfsid_params_apply_runtime(void)
{
    sid_set_model_filter_frequencies(kungfusid_parameters.filter_frequency_6581.value,
                                     kungfusid_parameters.filter_frequency_8580.value);
    sid_set_output_gain_percent(kungfusid_parameters.output_gain_percent.value);
}

void kfsid_params_init(void)
{
    kfsid_params_load_defaults();

    if (!kfsid_params_load_from_flash())
    {
        (void)kfsid_params_save_to_flash();
    }

    kfsid_params_apply_runtime();
}

uint16_t kfsid_params_count(void)
{
    return (uint16_t)kungfusid_params_table.n_params;
}

bool kfsid_param_get_value(uint16_t index, uint32_t* out_value)
{
    if (index >= kungfusid_params_table.n_params || out_value == NULL)
    {
        return false;
    }

    *out_value = kungfusid_params_table.params[index].value;
    return true;
}

bool kfsid_param_set_value(uint16_t index, uint32_t value)
{
    if (index >= kungfusid_params_table.n_params)
    {
        return false;
    }

    if (!xparam_set_value(&kungfusid_params_table.params[index], value))
    {
        return false;
    }

    kfsid_params_apply_runtime();
    return true;
}
