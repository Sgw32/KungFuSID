#ifndef EXT_EEPROM_H
#define EXT_EEPROM_H

#include <Arduino.h>
#include "xparam_eeprom.h"

extern digifiz_pars digifiz_parameters;
extern digifiz_pars default_parameters;
extern xparam_table_t params_table;

typedef enum
{
    EEPROM_NO_LOAD_ATTEMPT = 555,
    EEPROM_CORRUPTED = 800,
    EEPROM_OK_EXTERNAL = 100,
    EEPROM_OK_INTERNAL = 200,
    EEPROM_DEFAULT_STORED = 300,
} EEPROMLoadResult;

inline uint32_t dailyMileage(uint8_t block)
{
    return block ? digifiz_parameters.daily_mileage_1.value : digifiz_parameters.daily_mileage_0.value;
}

inline float averageConsumption(uint8_t block)
{
    return block ? digifiz_parameters.averageConsumption_1.value : digifiz_parameters.averageConsumption_0.value;
}

inline float averageSpeed(uint8_t block)
{
    return block ? digifiz_parameters.averageSpeed_1.value : digifiz_parameters.averageSpeed_0.value;
}

inline uint16_t durationMinutes(uint8_t block)
{
    return block ? digifiz_parameters.duration_1.value : digifiz_parameters.duration_0.value;
}

/**
 * @brief Loads defaults to the memory (internal or external EEPROM)
 *
 */
void load_defaults();

/**
 * @brief Inits EEPROM (internal or external)
 *
 */
void initEEPROM();

/**
 * @brief Save parameters e.g. while driving
 *
 */
void saveParameters();

/**
 * @brief Returns last EEPROM load result code
 */
EEPROMLoadResult getLoadResult();

#endif
