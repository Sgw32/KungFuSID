/*
 * Copyright (c) 2019-2022 Kim Jorgensen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "memory.h"
#include "kfsid_protocol.h"
#include "xparam_eeprom.h"
#include "hal.c"
#include "stm32f4xx/flash.c"
#include "print.c"
#include "file_types.h"
#include "xparam.c"
#include "xparam_eeprom.c"
#include "kfsid_protocol.c"
#include "usid.c"
#include "cartridges/ay3_backend.c"
#include "cartridges/emulator_backend.c"
#include "cartridge.c"
#include "math.h"

#define PI 3.14159259
#define SID_VDD_ADC_THRESHOLD 2235U

static void sid_configure_model_from_adc(void)
{
    u16 adc_value = adc_read_vdd_adc_pa4();

    if (adc_value > SID_VDD_ADC_THRESHOLD)
    {
        sid_apply_model(MOS6581);
    }
    else
    {
        sid_apply_model(MOS8580);
    }
}

static void adc_config(void)
{
    /* Configure PA3 as analog input (ADC1_IN3). */
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER3, GPIO_MODER_MODER3);
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD3, 0);

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    __DSB();

    MODIFY_REG(ADC->CCR, ADC_CCR_ADCPRE, ADC_CCR_ADCPRE_0);
    MODIFY_REG(ADC1->SMPR2, ADC_SMPR2_SMP3,
               ADC_SMPR2_SMP3_2 | ADC_SMPR2_SMP3_1);
    MODIFY_REG(ADC1->SQR1, ADC_SQR1_L, 0);
    MODIFY_REG(ADC1->SQR3, ADC_SQR3_SQ1, 3);

    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

static void sid_clock_config(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();

    TIM2->PSC = 168 / period;
    TIM2->ARR = SID_MULTIPLIER - 1;
    TIM2->EGR |= TIM_EGR_UG;

    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->SR &= ~TIM_SR_UIF;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    TIM2->SR &= ~TIM_SR_UIF;
    if (kfsid_protocol_audio_enabled())
    {
        emulator_backend_cycle_handler();
        DAC->DHR12R2 = main_volume;
    }
    else
    {
        DAC->DHR12R2 = 0;
    }
}

int main(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_EN2;

    configure_system();
    kfsid_params_init();
    emulator_backend_init();
    kfsid_protocol_init();
    pot_init();
    sid_configure_model_from_adc();
    adc_config();
    sid_clock_config();

    crt_ptr = CRT_LAUNCHER_BANK;
    kff_init();
    C64_INSTALL_HANDLER(kff_handler);
    c64_enable();

    while (true)
    {
        if (kfsid_protocol_restart_pending())
        {
            /* Let the C64 finish the final ACK bus cycle, then reboot only
             * KungFuSID. system_restart() also asserts the C64 reset line. */
            delay_ms(50);
            NVIC_SystemReset();
        }
    }
}
