#ifndef INC_PARAM_LIST_H_
#define INC_PARAM_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PARAM_LIST(PARAM) \
    PARAM( \
        U16, \
        filter_frequency_6581, \
        .p_name = "filter_frequency_6581", \
        .p_info = "6581 filter frequency (Hz)", \
        .min = 4000, \
        .max = 20000, \
        .value = 12500, \
    ) \
    PARAM( \
        U16, \
        filter_frequency_8580, \
        .p_name = "filter_frequency_8580", \
        .p_info = "8580 filter frequency (Hz)", \
        .min = 4000, \
        .max = 22000, \
        .value = 16000, \
    ) \
    PARAM( \
        U8, \
        output_gain_percent, \
        .p_name = "output_gain_percent", \
        .p_info = "Master output gain in percent", \
        .min = 10, \
        .max = 200, \
        .value = 100, \
    ) \
    PARAM( \
        U8, \
        default_sid_volume, \
        .p_name = "default_sid_volume", \
        .p_info = "SID volume nibble applied on reset", \
        .min = 0, \
        .max = 15, \
        .value = 15, \
    )

#ifdef __cplusplus
}
#endif

#endif /* INC_PARAM_LIST_H_ */
