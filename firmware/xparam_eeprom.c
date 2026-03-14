#include "xparam_eeprom.h"

#include "setup.h"

//static const size_t XPARAM_EEPR_PARAM_COUNT = sizeof(digifiz_pars) / sizeof(xparam_t);

const digifiz_pars default_parameters = {
    PARAM_LIST(DEFINE_PARAM)
};

digifiz_pars digifiz_parameters = {
    PARAM_LIST(DEFINE_PARAM)
};

xparam_table_t params_table = {
    .params = (xparam_t*)&digifiz_parameters,
    .n_params = (sizeof(digifiz_pars) / sizeof(xparam_t)),
};
