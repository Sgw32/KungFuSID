#include "emulator_backend.h"

#include "ay3_backend.h"
#include "irq.h"
#include "xparam_eeprom.h"

enum
{
    EMULATOR_BACKEND_SID = 0,
    EMULATOR_BACKEND_AY3 = 1,
};

static void sid_backend_write(uint8_t address, uint8_t value)
{
    setreg(address, value);
}

static emulator_write_handler_t emulator_backend_write_handler =
    sid_backend_write;
static emulator_cycle_handler_t emulator_backend_reset_handler = reset_SID;

emulator_cycle_handler_t emulator_backend_cycle_handler = SID_emulator;

void emulator_backend_init(void)
{
    /* Selection is intentionally performed once at boot. The audio IRQ only
     * makes an indirect call and never branches on the selected backend. */
    if (kungfusid_parameters.emulator_backend.value == EMULATOR_BACKEND_AY3)
    {
        emulator_backend_cycle_handler = ay3_backend_cycle;
        emulator_backend_write_handler = ay3_backend_write;
        emulator_backend_reset_handler = ay3_backend_reset;
    }
    else
    {
        emulator_backend_cycle_handler = SID_emulator;
        emulator_backend_write_handler = sid_backend_write;
        emulator_backend_reset_handler = reset_SID;
    }
    emulator_backend_reset();
}

void emulator_backend_reset(void)
{
    emulator_backend_reset_handler();
}

void emulator_backend_write(uint8_t address, uint8_t value)
{
    emulator_backend_write_handler(address, value);
}
