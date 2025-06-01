#define POTX_PIN     6
#define POTY_PIN     7
#define POTX_MASK    (1 << POTX_PIN)
#define POTY_MASK    (1 << POTY_PIN)

// Global variables
uint32_t potX_count = 0;
uint32_t potY_count = 0;

extern uint8_t POTX; // TODO
extern uint8_t POTY; // TODO

// Enum for measurement phases
typedef enum {
    PHASE_DISCHARGE = 0,
    PHASE_PREPARE_CHARGE = 1,
    PHASE_MEASURE_CHARGE = 2,
    PHASE_FINISH_CYCLE = 3
} PotMeasurementPhase;

// Measurement state
static PotMeasurementPhase pot_phase = PHASE_DISCHARGE;
static uint32_t discharge_count = 0;
static uint32_t measure_count = 0;

// Helper macros
#define SET_OUTPUT(pin)   (GPIOA->MODER = (GPIOA->MODER & ~(0x3 << ((pin)*2))) | (0x1 << ((pin)*2)))
#define SET_INPUT(pin)    (GPIOA->MODER &= ~(0x3 << ((pin)*2)))
#define WRITE_LOW(pin)    (GPIOA->ODR &= ~(1 << (pin)))
#define READ_PIN(pin)     ((GPIOA->IDR >> (pin)) & 0x1)


void pot_init(void)
{

}

FORCE_INLINE void pot_process(void)
{
    switch (pot_phase)
    {
        case PHASE_DISCHARGE:
            // Discharge pins by driving low
            SET_OUTPUT(POTX_PIN);
            SET_OUTPUT(POTY_PIN);
            WRITE_LOW(POTX_PIN);
            WRITE_LOW(POTY_PIN);

            // Count 255 discharge cycles
            discharge_count+=multiplier;
            if (discharge_count >= 255)
            {
                discharge_count = 0;
                pot_phase = PHASE_PREPARE_CHARGE;
            }
            break;

        case PHASE_PREPARE_CHARGE:
            // Set pins to input mode (start measuring charge time)
            SET_INPUT(POTX_PIN);
            SET_INPUT(POTY_PIN);

            potX_count = 0;
            potY_count = 0;
            measure_count = 0;

            pot_phase = PHASE_MEASURE_CHARGE;
            break;

        case PHASE_MEASURE_CHARGE:
            // For 255 cycles, count how long each pin stays LOW
            if (!READ_PIN(POTX_PIN) && potX_count < 255)
                potX_count+=multiplier;
            if (!READ_PIN(POTY_PIN) && potY_count < 255)
                potY_count+=multiplier;
            if ((READ_PIN(POTX_PIN) && READ_PIN(POTY_PIN)))
            {
                pot_phase = PHASE_FINISH_CYCLE;
            }
            if (measure_count >= 255)
            {
                pot_phase = PHASE_DISCHARGE;
            }
            measure_count+=multiplier;
            break;
        case PHASE_FINISH_CYCLE:
            SET_OUTPUT(POTX_PIN);
            SET_OUTPUT(POTY_PIN);
            WRITE_LOW(POTX_PIN);
            WRITE_LOW(POTY_PIN);
            if (measure_count >= 255)
            {
                pot_phase = PHASE_DISCHARGE;
            }
            measure_count+=multiplier;
        default:
            pot_phase = PHASE_DISCHARGE;
            discharge_count = 0;
            break;
    }

    if (potX_count<255)
        POTX = potX_count;
    else
        POTX = 255;   
    if (potY_count<255)
        POTY = potY_count;
    else
        POTY = 255; 
    // potX_count and potY_count hold the time-to-high in cycles 
}