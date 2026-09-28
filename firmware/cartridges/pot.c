#include "pot.h"

/*
 * The paddle capacitors are connected to PA6/PA7.  These pins are also
 * TIM3_CH1/TIM3_CH2, so input capture can timestamp the threshold crossings
 * in hardware.  This is both cheaper and more accurate than polling the pins
 * from the SID emulation interrupt (or timestamping a delayed GPIO IRQ).
 *
 * DWT->CYCCNT cannot be used as a free-running clock in this firmware: the
 * C64 bus handler deliberately reloads it from TIM1 every bus cycle.  TIM3
 * therefore runs at 1 MHz and its captured count is already a SID POT value.
 */
#define POT_PIN_X              6U
#define POT_PIN_Y              7U
#define POT_PIN_MASK           ((1U << POT_PIN_X) | (1U << POT_PIN_Y))
#define POT_TIMER_PRESCALER    83U     /* 84 MHz APB1 timer clock / 84 = 1 MHz */
#define POT_TIMER_MAX          255U

typedef enum
{
    POT_DISCHARGING,
    POT_MEASURING
} pot_phase_t;

static volatile pot_phase_t pot_phase;
static volatile uint8_t pot_x_capture;
static volatile uint8_t pot_y_capture;
static volatile uint8_t pot_capture_mask;

static inline void pot_discharge(void)
{
    /* Stop captures before changing the GPIO mux. */
    TIM3->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E);

    /* Select the low output level before enabling the output drivers. */
    GPIOA->BSRR = (POT_PIN_MASK << 16U);
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER6 | GPIO_MODER_MODER7,
                GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0);
}

static inline void pot_begin_measurement(void)
{
    pot_x_capture = POT_TIMER_MAX;
    pot_y_capture = POT_TIMER_MAX;
    pot_capture_mask = 0;

    /* Discard flags from the preceding phase and start the time base at zero. */
    TIM3->SR = ~(TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC1OF | TIM_SR_CC2OF);
    TIM3->CNT = 0;

    /* AF2 releases both capacitors and routes their edges to TIM3 capture. */
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER6 | GPIO_MODER_MODER7,
                GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);
    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
}

void pot_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    __DSB();

    /* No internal bias: the C64 paddle resistance charges the capacitors. */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
    GPIOA->OTYPER &= ~POT_PIN_MASK;

    /* PA6/PA7 alternate function 2 is TIM3_CH1/TIM3_CH2. */
    MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7,
                GPIO_AFRL_AFSEL6_1 | GPIO_AFRL_AFSEL7_1);

    TIM3->CR1 = 0;
    TIM3->PSC = POT_TIMER_PRESCALER;
    TIM3->ARR = POT_TIMER_MAX;

    /* Direct inputs with a short digital filter to reject narrow glitches. */
    TIM3->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0 |
                  TIM_CCMR1_IC1F_0 | TIM_CCMR1_IC1F_1 |
                  TIM_CCMR1_IC2F_0 | TIM_CCMR1_IC2F_1;
    TIM3->CCER = 0; /* Rising polarity; enabled when measurement begins. */
    TIM3->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC2IE;

    pot_phase = POT_DISCHARGING;
    pot_x_capture = POT_TIMER_MAX;
    pot_y_capture = POT_TIMER_MAX;
    pot_capture_mask = 0;
    pot_discharge();

    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR = 0;

    /* Bus timing remains highest priority; POT work preempts the audio IRQ. */
    NVIC_SetPriority(TIM3_IRQn, 1);
    NVIC_EnableIRQ(TIM3_IRQn);
    TIM3->CR1 = TIM_CR1_CEN;
}

void TIM3_IRQHandler(void)
{
    uint32_t status = TIM3->SR;
    TIM3->SR = ~(status & (TIM_SR_UIF | TIM_SR_CC1IF | TIM_SR_CC2IF |
                           TIM_SR_CC1OF | TIM_SR_CC2OF));

    if (pot_phase == POT_MEASURING && !(status & TIM_SR_UIF))
    {
        if ((status & TIM_SR_CC1IF) && !(pot_capture_mask & 1U))
        {
            pot_x_capture = (uint8_t)TIM3->CCR1;
            pot_capture_mask |= 1U;
            TIM3->DIER &= ~TIM_DIER_CC1IE;
        }
        if ((status & TIM_SR_CC2IF) && !(pot_capture_mask & 2U))
        {
            pot_y_capture = (uint8_t)TIM3->CCR2;
            pot_capture_mask |= 2U;
            TIM3->DIER &= ~TIM_DIER_CC2IE;
        }
    }

    if (status & TIM_SR_UIF)
    {
        TIM3->CNT = 0;

        if (pot_phase == POT_DISCHARGING)
        {
            pot_phase = POT_MEASURING;
            TIM3->DIER |= TIM_DIER_CC1IE | TIM_DIER_CC2IE;
            pot_begin_measurement();
        }
        else
        {
            /* Publish once per completed measurement, outside the audio IRQ. */
            POTX = pot_x_capture;
            POTY = pot_y_capture;
            SID[25] = pot_x_capture;
            SID[26] = pot_y_capture;
            pot_phase = POT_DISCHARGING;
            pot_discharge();
        }
    }
}
