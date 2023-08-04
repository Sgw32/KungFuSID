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
uint32_t sine_value[255];
uint32_t timer_data = 0;
//Timer2 Prescaler :2; Preload = 55999; Actual Interrupt Time = 1 ms
 
static void sid_clock_config()
{
     // Enable TIM1 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();
    // No prescaler, timer runs at ABP2 timer clock speed (168 MHz)
    TIM2->PSC = 81;
    TIM2->ARR = 15;
    TIM2->EGR |= TIM_EGR_UG;
    // Enable TIM1_CC_IRQn, highest priority
    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);
    // Enable counter
    TIM2->SR &= ~TIM_SR_UIF;
    //Enable the hardware interrupt.
    TIM2->DIER |= TIM_DIER_UIE;
    //Enable the timer.
    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
  TIM2->SR &= ~TIM_SR_UIF;
  SID_emulator();
  //DAC->DHR12R2 = main_volume;
  //timer_data+=1;
  DAC->DHR12R2 = main_volume; //((sin((float)timer_data*2.0f*PI/200.0f) + 1.0f)*(4096.0f/2.0f)); // формула взята из мануала на f4 серию
}

int main(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_EN2; // Channel 2
    reset_SID();
    OSC_1_HiLo = 7493; // 440 Hz
    MASTER_VOLUME  = 0x0f;
    ADSR_Attack_1  = 0x00;
    ADSR_Decay_1  = 0x00;
    ADSR_Sustain_1 = 0x0f;
    ADSR_Release_1 = 0x0f;
    PW_HiLo_voice_1 = 0x400;
    //sawtooth_bit_voice_1=1;
    triangle_bit_voice_1 = 0;
    pulse_bit_voice_1 = 1;
    Gate_bit_1 = 1;       
    configure_system();
    sid_clock_config();
    //c64_launcher_mode();
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
