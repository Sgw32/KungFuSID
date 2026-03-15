#ifndef XPARAM_EEPROM_H
#define XPARAM_EEPROM_H

#include <stdbool.h>
#include <stdint.h>
#include "xparam.h"
#include "params_list.h"

typedef struct
{
    PARAM_LIST(DECLARE_PARAM)
} kungfusid_parameters_t;

extern const kungfusid_parameters_t kungfusid_default_parameters;
extern kungfusid_parameters_t kungfusid_parameters;
extern xparam_table_t kungfusid_params_table;

void kfsid_params_load_defaults(void);
bool kfsid_params_load_from_flash(void);
bool kfsid_params_save_to_flash(void);
void kfsid_params_init(void);
void kfsid_params_apply_runtime(void);

uint16_t kfsid_params_count(void);
bool kfsid_param_get_value(uint16_t index, uint32_t* out_value);
bool kfsid_param_set_value(uint16_t index, uint32_t value);

#endif
