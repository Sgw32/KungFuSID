/*
 * Copyright (c) 2019-2022 Kim Jørgensen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
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
#include "hal.c"
#include "print.c"     
#include "file_types.h"
#include "usid.c"
#include "cartridge.c"
#include "math.h"


#define PI 3.14159259


/**
 * @brief Config DAC SID clock
 * @details Timer2 clocked from PHI2 (TIM1 TRGO) and used for SID ticks
 */
static void sid_clock_config()
{
     // Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();
    // External clock mode 1, trigger on TIM1 TRGO (ITR1)
    MODIFY_REG(TIM2->SMCR, TIM_SMCR_SMS|TIM_SMCR_TS,
               TIM_SMCR_SMS_2|TIM_SMCR_SMS_1|TIM_SMCR_SMS_0|
               TIM_SMCR_TS_0);
    // Set prescaler and auto-reload for SID tick rate
    TIM2->PSC = 0;
    TIM2->ARR = multiplier - 1;
    TIM2->EGR |= TIM_EGR_UG;
    // Enable TIM2_IRQn
    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);
    // Enable counter
    TIM2->SR &= ~TIM_SR_UIF;
    // Enable the hardware interrupt.
    TIM2->DIER |= TIM_DIER_UIE;
    // Enable the timer.
    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief SID DAC and emulation IRQ handler
 * 
 */
void TIM2_IRQHandler(void) {
  TIM2->SR &= ~TIM_SR_UIF;
  SID_emulator();
  DAC->DHR12R2 = main_volume;
}

/**
 * @brief main
 * 
 * @return int 
 */
int main(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_EN2; // Channel 2
    reset_SID();      
    configure_system();
    sid_clock_config();
    crt_ptr = CRT_LAUNCHER_BANK;
    kff_init();
    C64_INSTALL_HANDLER(kff_handler);
    c64_enable();
    
    /* PA5 Init  */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    /* Set GPIO PUPD register */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0x03 << (2 * (0x1UL << GPIO_BSRR_BS5)))) | ((uint32_t)(0x00 << (2 * GPIO_BSRR_BS5)));  
    /* Set GPIO MODE register */
    GPIOA->MODER = (GPIOA->MODER & ~((uint32_t)(0x03 << (2 * GPIO_BSRR_BS5)))) | ((uint32_t)(0x03 << (2 * GPIO_BSRR_BS5)));  

    while (true)
    {
       
    }
}
