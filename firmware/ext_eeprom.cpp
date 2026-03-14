#include "ext_eeprom.h"

#include <EEPROM.h>
#include <SparkFun_External_EEPROM.h>
#include <Wire.h>
#include <string.h>

#include "setup.h"

static constexpr size_t XPARAM_PARAM_COUNT = sizeof(digifiz_pars) / sizeof(xparam_t);
static constexpr size_t XPARAM_BLOB_SIZE = XPARAM_IMAGE_SIZE(XPARAM_PARAM_COUNT);
static constexpr uint16_t EEPROM_STORAGE_OFFSET = 0;

ExternalEEPROM myMem;
static bool external_faulty = false;
static EEPROMLoadResult eeprom_load_result = EEPROM_NO_LOAD_ATTEMPT;

static void writeBlobToInternal(const uint8_t* blob, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        EEPROM.update(EEPROM_STORAGE_OFFSET + i, blob[i]);
    }
}

static bool readBlobFromInternal(uint8_t* blob, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        blob[i] = EEPROM.read(EEPROM_STORAGE_OFFSET + i);
    }
    return true;
}

static void writeBlobToExternal(const uint8_t* blob, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        myMem.write(EEPROM_STORAGE_OFFSET + i, blob[i]);
    }
}

static bool readBlobFromExternal(uint8_t* blob, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        blob[i] = myMem.read(EEPROM_STORAGE_OFFSET + i);
    }
    return true;
}

void load_defaults()
{
    memcpy(&digifiz_parameters,
           &default_parameters,
           sizeof(digifiz_parameters));
}

void saveParameters()
{
#ifdef DISABLE_EEPROM
    return;
#endif
    uint8_t* blob_data = xparam_table_to_blob(&params_table);
    if (!blob_data)
    {
        return;
    }

    writeBlobToInternal(blob_data, XPARAM_BLOB_SIZE);
    if (!external_faulty)
    {
        writeBlobToExternal(blob_data, XPARAM_BLOB_SIZE);
    }
    eeprom_load_result = external_faulty ? EEPROM_OK_INTERNAL : EEPROM_OK_EXTERNAL;

    free(blob_data);
}

void initEEPROM()
{
    external_faulty = false;
    load_defaults();
 
#ifdef DISABLE_EEPROM
    return;
#endif

    Wire.begin();
    if (!myMem.begin())
    {
        external_faulty = true;
    }

    uint8_t* blob = (uint8_t*)malloc(XPARAM_BLOB_SIZE);
    if (!blob)
    {
        return;
    }

    bool loaded = false;
    if (!external_faulty)
    {
        readBlobFromExternal(blob, XPARAM_BLOB_SIZE);
        loaded = xparam_table_from_blob(&params_table, blob);
        if (loaded)
        {
            eeprom_load_result = EEPROM_OK_EXTERNAL;
        }
    }

    if (!loaded)
    {
        readBlobFromInternal(blob, XPARAM_BLOB_SIZE);
        loaded = xparam_table_from_blob(&params_table, blob);
        if (loaded)
        {
            eeprom_load_result = EEPROM_OK_INTERNAL;
        }
    }

    if (!loaded)
    {
        load_defaults();
        saveParameters();
        eeprom_load_result = EEPROM_DEFAULT_STORED;
    }

    free(blob);
}

EEPROMLoadResult getLoadResult()
{
    return eeprom_load_result;
}
