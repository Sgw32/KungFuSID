#ifndef XPARAM_EEPROM_H
#define XPARAM_EEPROM_H

#include "xparam.h"
#include "params_list.h"

typedef struct
{
    PARAM_LIST(DECLARE_PARAM)
} digifiz_pars;

extern xparam_table_t params_table;

#endif
