#include "setup.h"
#include "irq.h"
#include "sid.h"

/**
 * @brief startup sound
 * 
 */
void error_open_folder(void) 
{
  reset_SID();
}


/**
 * @brief Sets SID register
 * 
 * @param addr 
 * @param value 
 */
void setreg(uint8_t addr,uint8_t value)
{
    uint8_t access_adress = addr;
    SID[(access_adress)] = value;

    switch (access_adress) {
      case 0:
        OSC_1_HiLo = ((SID[0] & 0xff) + ( (SID[1] & 0xff) << 8) ); // *0.985
        break;
      case 1:
        OSC_1_HiLo = ((SID[0] & 0xff) + ( (SID[1] & 0xff) << 8)); // *0.985
        break;
      case 2:
        PW_HiLo_voice_1 = SID[2] + (((SID[3] & 0x0f) << 8 ));
        break;
      case 3:
        PW_HiLo_voice_1 = SID[2] + (( (SID[3] & 0x0f) << 8 ));
        break;
      case 4:
        STAD4XX = 1;
        Gate_bit_1 = SID[4] & 1;   //
        break;
      case 5:
        ADSR_Attack_1 = ( (SID[5] >> 4 ) & 0x0f) ;
        ADSR_Decay_1 = ( (SID[5]  ) & 0x0f) ;
        break;
      case 6:
        ADSR_Sustain_1 = ( (SID[6] >> 4 ) & 0x0f) ;
        ADSR_Release_1 = ( (SID[6]  ) & 0x0f);
        break;
      case 7:
        OSC_2_HiLo = (SID[7] + ( SID[8] << 8));
        break;
      case 8:
        OSC_2_HiLo = (SID[7] + ( SID[8] << 8)); 
        break;
      case 9:
        PW_HiLo_voice_2 = SID[9] + ( (SID[10] & 0x0f) << 8);
        break;
      case 10:
        PW_HiLo_voice_2 = SID[9] + ( (SID[10] & 0x0f) << 8);
        break;
      case 11:
        STAD4XX = 1;
        Gate_bit_2 = SID[11] & 1;   //
        break;
      case 12:
        ADSR_Attack_2 = ( (SID[12] >> 4 ) & 0x0f) ;
        ADSR_Decay_2 = ( (SID[12]  ) & 0x0f) ;
        break;
      case 13:
        ADSR_Sustain_2 = ( (SID[13] >> 4 ) & 0x0f) ;
        ADSR_Release_2 = ( (SID[13]  ) & 0x0f);
        break;
      case 14:
        OSC_3_HiLo = (SID[14] + ( SID[15] << 8));
        break;
      case 15:
        OSC_3_HiLo = (SID[14] + ( SID[15] << 8));
        break;
      case 16:
        PW_HiLo_voice_3 = SID[16] + ( (SID[17] & 0x0f) << 8);
        break;
      case 17:
        PW_HiLo_voice_3 = SID[16] + ( (SID[17] & 0x0f) << 8);
        break;
      case 18:
        STAD4XX = 1;
        Gate_bit_3 = SID[18] & 1;
        break;
      case 19:
        ADSR_Attack_3 = ( (SID[19] >> 4 ) & 0x0f) ;
        ADSR_Decay_3 = ( (SID[19]  ) & 0x0f) ;
        break;
      case 20:
        ADSR_Sustain_3 = ( (SID[20] >> 4 ) & 0x0f) ;
        ADSR_Release_3 = ( (SID[20]  ) & 0x0f);
        break;
      case 21:
        FILTER_HiLo = (SID[21] & 0x07) + ( SID[22] << 3); // 11bit // TODO

        //set w0
        // from reSID
        // Multiply with 1.048576 to facilitate division by 1 000 000 by right-
        // shifting 20 times (2 ^ 20 = 1048576).
        // w0 = static_cast<sound_sample>(2*pi*f0[fc]*1.048576);
        w0 = w0_constant_part * (FILTER_HiLo + 0x01); // 0x01 offset testing
        // w0_ceil_dt = w0 <= w0_max_dt ? w0 : w0_max_dt;
        if (w0 < w0_max_dt) {
          w0_ceil_dt = w0;
        }
        else {
          w0_ceil_dt = w0_max_dt;
        }

        break;
      case 22:
        FILTER_HiLo = (SID[21] & 0x07) + (SID[22] << 3); // 11bit // TODO

        //set w0
        // Multiply with 1.048576 to facilitate division by 1 000 000 by right-
        // shifting 20 times (2 ^ 20 = 1048576).
        // w0 = static_cast<sound_sample>(2*pi*f0[fc]*1.048576);
        w0 = w0_constant_part * (FILTER_HiLo + 0x1); //

        // w0_ceil_dt = w0 <= w0_max_dt ? w0 : w0_max_dt;
        if (w0 < w0_max_dt) {
          w0_ceil_dt = w0;
        }
        else {
          w0_ceil_dt = w0_max_dt;
        }

        break;
      case 23:
        FILTER_Resonance = ( (SID[23] >> 4 ) & 0x0f) ;; // 4bit // TODO
        
        // The coefficient 1024 is dispensed of later by right-shifting 10 times
        // (2 ^ 10 = 1024).
        // _1024_div_Q = static_cast<sound_sample>(1024.0/(0.707 + 1.0*res/0x0f));
        Q_1024_div = (1024.0 / (0.707 + 1.0 * (FILTER_Resonance) / 15.0));
        break;
      case 24:
        OFF3 =  ( (SID[24] >> 7 ) & 1) ;; // on/off; //
        break;
      default:
        break;
    }

    // STAD4XX = 1; // SID write signal for IRQ
    //PB13_HIGH;
}

/**
 * @brief Resets SID state
 * 
 */

void reset_SID()
{
  // list of global variables that control SID emulator . Keep in mind that IRQ will detect change after some delay (around 50uS)
  // detailed information: http://archive.6502.org/datasheets/mos_6581_sid.pdf

  // channel 1
  OSC_1_HiLo            = 0;              // 0-65535      // 
  PW_HiLo_voice_1       = 0;              // 0-4095       // 
  Gate_bit_1            = 0;              // true/false   // 
  ADSR_Attack_1         = 0;              // 0-15         // 
  ADSR_Decay_1          = 0;              // 0-15         // 
  ADSR_Sustain_1        = 0;              // 0-15         // 
  ADSR_Release_1        = 0;              // 0-15         // 

  // channel 2

  OSC_2_HiLo            = 0;              // 0-65535      // 
  PW_HiLo_voice_2       = 0;              // 0-4095       // 
  Gate_bit_2            = 0;              // true/false   // 
  ADSR_Attack_2         = 0;              // 0-15         // 
  ADSR_Decay_2          = 0;              // 0-15         // 
  ADSR_Sustain_2        = 0;              // 0-15         // 
  ADSR_Release_2        = 0;              // 0-15         // 

  // channel 3
  OSC_3_HiLo            = 0;              // 0-65535      // 
  PW_HiLo_voice_3       = 0;              // 0-4095       // 
  Gate_bit_3            = 0;              // true/false   // 
  ADSR_Attack_3         = 0;              // 0-15         // 
  ADSR_Decay_3          = 0;              // 0-15         // 
  ADSR_Sustain_3        = 0;              // 0-15         // 
  ADSR_Release_3        = 0;              // 0-15         // 

  // other registers
  FILTER_HiLo           = 0;              // 0-2047       // 
  FILTER_Resonance      = 0;              // 0-15         // 
  OFF3                  = 0;              // true/false   // 
  memset(SID, 0, sizeof(SID));
}

/**
 * @brief Main emulator function which outputs to DAC
 * 
 */
void SID_emulator ()
{
    OSC_MSB_Previous_1 = OSC_MSB_1;
    OSC_MSB_Previous_2 = OSC_MSB_2;
    OSC_MSB_Previous_3 = OSC_MSB_3;

    OSC_1 = (~(SID[4] >> 3 ) & 1) * ((OSC_1 + (  multiplier * OSC_1_HiLo)) ) & 0xffffff;
    OSC_2 = (~(SID[11] >> 3 ) & 1) * ((OSC_2 + (  multiplier * OSC_2_HiLo)) ) & 0xffffff;
    OSC_3 = (~(SID[18] >> 3 ) & 1) * ((OSC_3 + (  multiplier * OSC_3_HiLo)) ) & 0xffffff;

    // noise_1
    OSC_noise_1 = OSC_noise_1 + multiplier * OSC_1_HiLo; // noise counter (
    OSC_bit19_1 = OSC_noise_1 >> 19 ; //  / 0x080000;// calculate how many missing rising edges of bit_19 since last irq (if any)
    for (i = 0; i < OSC_bit19_1; i++) {
      bit_0_1 = (( bitRead(pseudorandom_1, 22)   ) ^ ((bitRead(pseudorandom_1, 17 ) ) )  ) & 0x1;
      pseudorandom_1 = pseudorandom_1 << 1;
      pseudorandom_1 = bit_0_1 | pseudorandom_1;
    }
    OSC_noise_1 = OSC_noise_1 - (OSC_bit19_1 << 19); // * 0x080000); // no reset, keep lower 18bit


    // noise_2
    OSC_noise_2 = OSC_noise_2 + multiplier * OSC_2_HiLo; // noise counter (
    OSC_bit19_2 = OSC_noise_2 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    for (i = 0; i < OSC_bit19_2; i++) {
      bit_0_2 = (( bitRead(pseudorandom_2, 22)   ) ^ ((bitRead(pseudorandom_2, 17 ) ) )  ) & 0x1;
      pseudorandom_2 = pseudorandom_2 << 1;
      pseudorandom_2 = bit_0_2 | pseudorandom_2;
    }
    OSC_noise_2 = OSC_noise_2 - (OSC_bit19_2 << 19) ; // * 0x080000); // no reset, keep lower 18bits

    // noise_3
    OSC_noise_3 = OSC_noise_3 + multiplier * OSC_3_HiLo; // noise counter (
    OSC_bit19_3 = OSC_noise_3 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    for (i = 0; i < OSC_bit19_3; i++) {
      bit_0_3 = (( bitRead(pseudorandom_3, 22)   ) ^ ((bitRead(pseudorandom_3, 17 ) ) )  ) & 0x1;
      pseudorandom_3 = pseudorandom_3 << 1;
      pseudorandom_3 = bit_0_3 | pseudorandom_3;
    }
    OSC_noise_3 = OSC_noise_3 - (OSC_bit19_3 << 19 ); //  * 0x080000); // no reset, keep lower 18bit


    if (OSC_1 >= 0x800000)     OSC_MSB_1 = 1; else OSC_MSB_1 = 0;
    if ( (!OSC_MSB_Previous_1) & (OSC_MSB_1)) MSB_Rising_1 = 0b10; else  MSB_Rising_1 = 0;

    if (OSC_2 >= 0x800000)     OSC_MSB_2 = 1; else OSC_MSB_2 = 0;
    if ( (!OSC_MSB_Previous_2) & (OSC_MSB_2)) MSB_Rising_2 = 0b10; else  MSB_Rising_2 = 0;

    if (OSC_3 >= 0x800000)     OSC_MSB_3 = 1; else OSC_MSB_3 = 0;
    if ( (!OSC_MSB_Previous_3) & (OSC_MSB_3)) MSB_Rising_3 = 0b10; else MSB_Rising_3 = 0;


    // SYNC (bit 0b10)
    if ((SID[4] & 0b10 ) & MSB_Rising_3) OSC_1 = OSC_1 & 0x7fffff; // ANDing curent value of OSC with  0x7fffff  i get exact timing when sync happened, no matter of multiplier (and what's more important, what number is in OSC in this exact time)
    if ((SID[11] & 0b10 ) & MSB_Rising_1) OSC_2 = OSC_2 & 0x7fffff;
    if ((SID[18] & 0b10 ) & MSB_Rising_2) OSC_3 = OSC_3 & 0x7fffff;

    //Triangle and ringmod
    if ( (SID[4]&0b10100)) OSC_MSB_1 = OSC_MSB_1 ^ OSC_MSB_3; // this one took really long time to figure it out. I tought OSC_MSB_1 =  OSC_MSB_3 and everything was wacky with ring modulation
    if ( (SID[11]&0b10100)) OSC_MSB_2 = OSC_MSB_2 ^ OSC_MSB_1; // TODO: see if it's exact on high frequencies
    if ( (SID[18]&0b10100)) OSC_MSB_3 = OSC_MSB_3 ^ OSC_MSB_2; // TODO: see what's faster, here or in triangle voice check

    waveform_switch_1 = SID[4]&0xF0;
    waveform_switch_2 = SID[11]&0xF0;
    waveform_switch_3 = SID[18]&0xF0;

    temp11 = (OSC_1 >> 12); // upper 12 bit of OSC_1 calculate once now

    switch (waveform_switch_1) {
      case 0b00000000:
        WaveformDA_1 = 0;
        break;
      case 0b00010000:
        WaveformDA_triangle_1 = ((  (OSC_MSB_1 * B2047) ^ (temp11 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_1 = WaveformDA_triangle_1;
        break;
      case 0b00100000:
        WaveformDA_sawtooth_1 = temp11; // same as upper 12 bits of OSC
        WaveformDA_1 = WaveformDA_sawtooth_1;
        break;
      case 0b00110000:
        WaveformDA_triangle_1 = ((  (OSC_MSB_1 * B2047) ^ (temp11 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_1 = temp11; // same as upper 12 bits of OSC
        WaveformDA_1 = AND_mask[(WaveformDA_triangle_1 & WaveformDA_sawtooth_1)] << 4; // combined waveform. AND-ed value is take from array (array is actually combined waveform of sawtooth and pulse of 0 value (maximum DC) )
        break;
      case 0b01000000:
        if (temp11 >= PW_HiLo_voice_1 )  WaveformDA_pulse_1 = B4095; else WaveformDA_pulse_1 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_1 = WaveformDA_pulse_1;
        break;
      case 0b01010000:
        WaveformDA_triangle_1 = ((  (OSC_MSB_1 * B2047) ^ (temp11 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        if (temp11 >= PW_HiLo_voice_1 )  WaveformDA_pulse_1 = B4095; else WaveformDA_pulse_1 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_1 = AND_mask[WaveformDA_triangle_1 & WaveformDA_pulse_1] << 4;
        break;
      case 0b01100000:
        WaveformDA_sawtooth_1 = temp11; // same as upper 12 bits of OSC
        if (temp11 >= PW_HiLo_voice_1 )  WaveformDA_pulse_1 = B4095; else WaveformDA_pulse_1 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_1 = AND_mask[WaveformDA_sawtooth_1 & WaveformDA_pulse_1] << 4;
        break;
      case 0b01110000:
        WaveformDA_triangle_1 = ((  (OSC_MSB_1 * B2047) ^ (temp11 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_1 = temp11;
        if (temp11 >= PW_HiLo_voice_1 )  WaveformDA_pulse_1 = B4095; else WaveformDA_pulse_1 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_1 = AND_mask[WaveformDA_pulse_1 & WaveformDA_sawtooth_1 & WaveformDA_triangle_1] << 4;
        break;
      case 0b10000000:
        WaveformDA_noise_1 = B4095 & (pseudorandom_1 >> 11);
        WaveformDA_1 =  WaveformDA_noise_1;
        break;
      default:
        WaveformDA_1 = 0;
        break;

    }
    // end of voice 1
    // voice 2

    temp12 = (OSC_2 >> 12); // upper 12 bit of OSC_2 calculate once now

    switch (waveform_switch_2) {
      case 0b00000000:
        WaveformDA_2 = 0;
        break;
      case 0b00010000:
        WaveformDA_triangle_2 = ((  (OSC_MSB_2 * B2047) ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_2 = WaveformDA_triangle_2;
        break;
      case 0b00100000:
        WaveformDA_sawtooth_2 = temp12;
        WaveformDA_2 = WaveformDA_sawtooth_2;
        break;
      case 0b00110000:
        WaveformDA_triangle_2 = ((  (OSC_MSB_2 * B2047) ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_2 = temp12;
        WaveformDA_2 = AND_mask[(WaveformDA_triangle_2 & WaveformDA_sawtooth_2)] << 4;
        break;
      case 0b01000000:
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = WaveformDA_pulse_2;
        break;
      case 0b01010000:
        WaveformDA_triangle_2 = ((  (OSC_MSB_2 * B2047) ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = AND_mask[WaveformDA_triangle_2 & WaveformDA_pulse_2] << 4;
        break;
      case 0b01100000:
        WaveformDA_sawtooth_2 = temp12;
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = AND_mask[WaveformDA_sawtooth_2 & WaveformDA_pulse_2] << 4;
        break;
      case 0b01110000:
        WaveformDA_triangle_2 = ((  (OSC_MSB_2 * B2047) ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_2 = temp12;
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = AND_mask[WaveformDA_pulse_2 & WaveformDA_sawtooth_2 & WaveformDA_triangle_2] << 4;
        break;
      case 0b10000000:
        WaveformDA_noise_2 = B4095 & (pseudorandom_2 >> 11);
        WaveformDA_2 =  WaveformDA_noise_2;
        break;
      default:
        WaveformDA_2 = 0;
        break;

    }
    // end of voice 2


    temp13 = (OSC_3 >> 12); // upper 12 bit of OSC_3 calculate once now

    switch (waveform_switch_3) {
      case 0b00000000:
        WaveformDA_3 = 0;
        break;
      case 0b00010000:
        WaveformDA_triangle_3 = ((  (OSC_MSB_3 * B2047) ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_3 = WaveformDA_triangle_3;
        break;
      case 0b00100000:
        WaveformDA_sawtooth_3 = temp13;
        WaveformDA_3 = WaveformDA_sawtooth_3;
        break;
      case 0b00110000:
        WaveformDA_triangle_3 = ((  (OSC_MSB_3 * B2047) ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_3 = temp13;
        WaveformDA_3 = AND_mask[(WaveformDA_triangle_3 & WaveformDA_sawtooth_3)] << 4;
        break;
      case 0b01000000:
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = WaveformDA_pulse_3;
        break;
      case 0b01010000:
        WaveformDA_triangle_3 = ((  (OSC_MSB_3 * B2047) ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = AND_mask[WaveformDA_triangle_3 & WaveformDA_pulse_3] << 4;
        break;
      case 0b01100000:
        WaveformDA_sawtooth_3 = temp13;
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = AND_mask[WaveformDA_sawtooth_3 & WaveformDA_pulse_3] << 4;
        break;
      case 0b01110000:
        WaveformDA_triangle_3 = ((  (OSC_MSB_3 * B2047) ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_3 = temp13;
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = AND_mask[WaveformDA_pulse_3 & WaveformDA_sawtooth_3 & WaveformDA_triangle_3] << 4;
        break;
      case 0b10000000:
        WaveformDA_noise_3 = B4095 & (pseudorandom_3 >> 11);
        WaveformDA_3 =  WaveformDA_noise_3;
        break;
      default:
        WaveformDA_3 = 0;
        break;

    }
    // end of voice 3

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // gate change check
    //
    //  Gate_bit_voice_X variable: for now, main program can set it to any value , irq will reset it to 0
    //   0 - change from 1 to 0 * start Release stage
    //   1 - change from 0 to 1 * start Attack stage

    switch (Gate_bit_1) {
      case 0: // change from 1 to 0 * start Release stage
        if (Gate_previous_1 == 1) {
          ADSR_stage_1 = 4;// Set Release
          LFSR15_1 = 0; // preventing ADSR bugs
          LFSR5_1 = 0; // preventing ADSR bugs
          LFSR15_comparator_value_1 = ADSR_LFSR15[ADSR_Release_1];
          Gate_previous_1 = 0; // set to 0
        }
        break;
      case 1: // change from 0 to 1 * start Attack/Decay stage
        if (Gate_previous_1 == 0) {

          ADSR_stage_1 = 1; //
          LFSR15_1 = 0;
          LFSR5_1 = 0;
          Gate_previous_1 = 1; // set to 1
          // Switching to attack state unlocks the zero freeze.
          hold_zero_1 = false;
          LFSR15_comparator_value_1 = ADSR_LFSR15[ADSR_Attack_1];
        }
        break;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Increase LFSR15 counter for ADSR (scaled to match)

    //LFSR15_1 = LFSR15_1 & 0x7fff; // 15bit
    LFSR15_1 = LFSR15_1 + multiplier;;
    // LFSR15_1 = LFSR15_1 & 0x7fff; // 15bit

    if (   ((LFSR15_1 >= LFSR15_comparator_value_1 ) ) ) { //

      Divided_LFSR15_1 = ((LFSR15_1 ) / LFSR15_comparator_value_1); //  count how many time (LFSR15_1 == LFSR15_comparator_value_1 ) was skipped (it's unsigned integer, so no decimals
      LFSR15_1 = LFSR15_1 - Divided_LFSR15_1 * LFSR15_comparator_value_1; // set to zero, plus how manu uS has passed since (LFSR15_1 == LFSR15_comparator_value_1 )
      // LFSR15_1 = 0;
      //  LFSR5_1 = LFSR5_1++;
      // LFSR5_1 = LFSR5_1 + 1;
      LFSR5_1 = LFSR5_1 + Divided_LFSR15_1 ; // increase LFSR5 counter and check how many (LFSR5_1 == LFSR5_comparator_value_1) was skipped

      if ((ADSR_stage_1 == 1) | (LFSR5_1 >= LFSR5_comparator_value_1) ) {

        //  LFSR5_1 = 0;
        Divided_LFSR5_1 = (LFSR5_1 ) / LFSR5_comparator_value_1; //      ceo5 = (lfsr5 - previous5) / comp5 // comp5 in attack is always 1

        if (Divided_LFSR5_1 >= 1) {
          LFSR5_1 = 0;
        }
        else {
          LFSR5_1 =  LFSR5_1 - Divided_LFSR5_1 * LFSR5_comparator_value_1; //      set to zero, plus how manu uS has passed since (LFSR5_1 == LFSR5_comparator_value_1)
        }

        //     if (hold_zero_1) {
        //       return;
        //    }

        if (hold_zero_1 == false) {
          // 0-release finished , 1-Attack, 2-Decay, 3-Sustain, 4-Release
          switch (ADSR_stage_1) {

            case 0: // release finished, no change

              // lalalalala

              break;
            case 1: // Attack stage
              //  ADSR_volume_1 = (ADSR_volume_1 + 1) & 0xff;
              ADSR_volume_1 = (ADSR_volume_1 + Divided_LFSR15_1) ; // increase  volume value .


              if (ADSR_volume_1 >= 0xff) { // Attack finished, start Decay //
                // Due to time passed since last volume increase, this can be bigger then 0xff.
                // That means that decay stage already started and all values above 0xff is values that should be decrease in decay stage.
                // This is not perfect, those values should be decrementing at decay rate, not as attack rate
                // But is acceptable, because it would be inaccurate only for ADSR_attack values of 0 and 1, where LFSR15_comparator_value is smaller then multiplier.
                // So, timing of decay is off by maximum of <multiplier-1> uS.
                // Other values are decremented at steady -1 rate or less
                ADSR_volume_1 = 0xff - (ADSR_volume_1 - 0xff); // we are in decay stage already, calculate it's volume, depending of uS passed since (ADSR_volume_1 == 0xff)
                ADSR_stage_1 = 2; // set decay stage
                hold_zero_1 = false;
                LFSR15_comparator_value_1 = ADSR_LFSR15[ADSR_Decay_1   ]; // set counter comparator to match decay value

              }

              break;
            case 2: // Decay stage

              // 1 step of volume down
              //ADSR_volume_1 = ADSR_volume_1 - 1;
              if (ADSR_volume_1 >= Divided_LFSR5_1)        {
                ADSR_volume_1 = ADSR_volume_1 - Divided_LFSR5_1; //  keep it as positive number
              }
              else {
                ADSR_volume_1 = 0; // no negative numbers, clip it to zero
              }

              if (ADSR_volume_1 <= (((uint16_t) ADSR_Sustain_1 << 4) + ADSR_Sustain_1)) {
                ADSR_volume_1 = (( ADSR_Sustain_1 << 4) + ADSR_Sustain_1); // no matter what number of exact volume is passed, set it to exact value
                LFSR15_comparator_value_1 = ADSR_LFSR15[ADSR_Release_1   ]; // set counter comparator to match release value
                // decay finished, time to sustain
                ADSR_stage_1 = 3;

              }
              break;
            case 3: // Sustain stage - checking for change in sustain value
              if (ADSR_volume_1 > (((uint16_t) ADSR_Sustain_1 << 4) + ADSR_Sustain_1)) {

                // new sustain value is smaller then old,  get back to decay.

                ADSR_stage_1 = 2; // set Decay to new value
                LFSR15_comparator_value_1 = ADSR_LFSR15[ADSR_Decay_1   ]; // set counter comparator to match decay value
              }
              break;
            case 4: // Release stage

              //  ADSR_volume_1 = (ADSR_volume_1 - 1) & 0xff;
              if (ADSR_volume_1 >= Divided_LFSR5_1)        {
                ADSR_volume_1 = ADSR_volume_1 - Divided_LFSR5_1; //  keep it as positive number
              }
              else {
                ADSR_volume_1 = 0; // no negative numbers, clip it to zero
              }
              break;

          } // ADSR_stage switch


          LFSR5_comparator_value_1 = ADSR_Volume2LFSR5[ADSR_volume_1]; // must look into table, exact values are not possible


          if (ADSR_volume_1 == 0) {
            hold_zero_1 = true;
          }

        } // not hold zero
      }  // LFSR5_comparator_value check

      //


    }  // LFSR15_comparator_value check

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    //ADSR2
    // test point //


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // gate change check
    //
    //  Gate_bit_voice_X variable: for now, main program can set it to any value , irq will reset it to 0
    //   0 - change from 1 to 0 * start Release stage
    //   1 - change from 0 to 1 * start Attack stage


    //


    switch (Gate_bit_2) {
      case 0: // change from 1 to 0 * start Release stage

        if (Gate_previous_2 == 1) {
          ADSR_stage_2 = 4;// Set Release
          LFSR15_2 = 0;
          LFSR5_2 = 0;
          LFSR15_comparator_value_2 = ADSR_LFSR15[ADSR_Release_2];
          Gate_previous_2 = 0; // set to 0


        }



        break;
      case 1: // change from 0 to 1 * start Attack/Decay stage
        if (Gate_previous_2 == 0) {
          ADSR_stage_2 = 1; //
          LFSR15_2 = 0;
          LFSR5_2 = 0;
          Gate_previous_2 = 1; // set to 1
          // Switching to attack state unlocks the zero freeze.
          hold_zero_2 = false;
          LFSR15_comparator_value_2 = ADSR_LFSR15[ADSR_Attack_2];
        }


        break;

    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Increase LFSR15 counter for ADSR (scaled to match)



    LFSR15_2 = LFSR15_2 + multiplier;;
    // LFSR15_2 = LFSR15_2 & 0x7fff; // 15bit



    if (   ((LFSR15_2 >= LFSR15_comparator_value_2 ) ) ) { //

      Divided_LFSR15_2 = ((LFSR15_2 ) / LFSR15_comparator_value_2); //  count how many time (LFSR15_2 == LFSR15_comparator_value_2 ) was skipped (it's unsigned integer, so no decimals
      LFSR15_2 = LFSR15_2 - Divided_LFSR15_2 * LFSR15_comparator_value_2; // set to zero, plus how manu uS has passed since (LFSR15_2 == LFSR15_comparator_value_2 )
      // LFSR15_2 = 0;
      //  LFSR5_2 = LFSR5_2++;

      //

      // LFSR5_2 = LFSR5_2 + 1;
      LFSR5_2 = LFSR5_2 + Divided_LFSR15_2 ; // increase LFSR5 counter and

      if ((ADSR_stage_2 == 1) | (LFSR5_2 >= LFSR5_comparator_value_2) ) {

        //  LFSR5_2 = 0;
        Divided_LFSR5_2 = (LFSR5_2 ) / LFSR5_comparator_value_2; //      check how many (LFSR5_2 == LFSR5_comparator_value_2) was skipped

        if (Divided_LFSR5_2 >= 1) {
          LFSR5_2 = 0;
        }

        if (hold_zero_2 == false) {
          // 0-release finished , 1-Attack, 2-Decay, 3-Sustain, 4-Release
          switch (ADSR_stage_2) {

            case 0: // release finished, no change
              // lalalalala
              break;
            case 1: // Attack stage
              //  ADSR_volume_2 = (ADSR_volume_2 + 1) & 0xff;
              ADSR_volume_2 = (ADSR_volume_2 + Divided_LFSR15_2) ; // increase  volume value (value is unsigned 8bit, but variable is 16bit unsigned integer ), incliding skipped steps ;

              if (ADSR_volume_2 >= 0xff) { // Attack finished, start Decay //
                ADSR_volume_2 = 0xff - (ADSR_volume_2 - 0xff); // we are in decay stage already, calculate it's volume, depending of uS passed since (ADSR_volume_2 == 0xff)
                ADSR_stage_2 = 2; // set decay stage
                hold_zero_2 = false;
                LFSR15_comparator_value_2 = ADSR_LFSR15[ADSR_Decay_2   ]; // set counter comparator to match decay value

              }

              break;
            case 2: // Decay stage

              // 1 step of volume down
              //ADSR_volume_2 = ADSR_volume_2 - 1;
              if (ADSR_volume_2 >= Divided_LFSR5_2)        {
                ADSR_volume_2 = ADSR_volume_2 - Divided_LFSR5_2; //  keep it as positive number
              }
              else {
                ADSR_volume_2 = 0; // no negative numbers, clip it to zero
              }

              if (ADSR_volume_2 <= (( ADSR_Sustain_2 << 4) + ADSR_Sustain_2)) {
                ADSR_volume_2 = (( ADSR_Sustain_2 << 4) + ADSR_Sustain_2);
                LFSR15_comparator_value_2 = ADSR_LFSR15[ADSR_Release_2   ]; // set counter comparator to match release value
                // decay finished, time to sustain
                ADSR_stage_2 = 3;

              }
              break;
            case 3: // Sustain stage - checking for change in sustain value
              if (ADSR_volume_2 > (( ADSR_Sustain_2 << 4) + ADSR_Sustain_2)) {
                // new sustain value is smaller then old,  get back to decay.

                ADSR_stage_2 = 2; // set Decay to new value
                LFSR15_comparator_value_2 = ADSR_LFSR15[ADSR_Decay_2   ]; // set counter comparator to match decay value
              }
              break;
            case 4: // Release stage

              //  ADSR_volume_2 = (ADSR_volume_2 - 1) & 0xff;
              if (ADSR_volume_2 >= Divided_LFSR5_2)        {
                ADSR_volume_2 = ADSR_volume_2 - Divided_LFSR5_2; //  keep it as positive number
              }
              else {
                ADSR_volume_2 = 0; // no negative numbers, clip it to zero
              }
              break;

          } // ADSR_stage switch


          LFSR5_comparator_value_2 = ADSR_Volume2LFSR5[ADSR_volume_2]; // must look into 8bit table, exact values are not possible


          if (ADSR_volume_2 == 0) {
            hold_zero_2 = true;
          }

        } // not hold zero
      }  // LFSR5_comparator_value check

      //


    }  // LFSR15_comparator_value check


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    //ADSR3


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // gate change check
    //
    //  Gate_bit_voice_X variable: for now, main program can set it to any value , irq will reset it to 0
    //   0 - change from 1 to 0 * start Release stage
    //   1 - change from 0 to 1 * start Attack stage
    //    2 - no change

    //


    switch (Gate_bit_3) {
      case 0: // change from 1 to 0 * start Release stage

        if (Gate_previous_3 == 1) {
          ADSR_stage_3 = 4;// Set Release
          LFSR15_3 = 0;
          LFSR5_3 = 0;
          LFSR15_comparator_value_3 = ADSR_LFSR15[ADSR_Release_3];
          Gate_previous_3 = 0; // set to 0

        }



        break;
      case 1: // change from 0 to 1 * start Attack/Decay stage
        if (Gate_previous_3 == 0) {
          ADSR_stage_3 = 1; //
          LFSR15_3 = 0;
          LFSR5_3 = 0;
          Gate_previous_3 = 1; // set to 1
          // Switching to attack state unlocks the zero freeze.
          hold_zero_3 = false;
          LFSR15_comparator_value_3 = ADSR_LFSR15[ADSR_Attack_3];
        }


        break;

    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    // Increase LFSR15 counter for ADSR (scaled to match)



    LFSR15_3 = LFSR15_3 + multiplier;;
    // LFSR15_3 = LFSR15_3 & 0x7fff; // 15bit



    if (   ((LFSR15_3 >= LFSR15_comparator_value_3 ) ) ) { //

      Divided_LFSR15_3 = ((LFSR15_3 ) / LFSR15_comparator_value_3); //  count how many time (LFSR15_3 == LFSR15_comparator_value_3 ) was skipped (it's unsigned integer, so no decimals
      LFSR15_3 = LFSR15_3 - Divided_LFSR15_3 * LFSR15_comparator_value_3; // set to zero, plus how manu uS has passed since (LFSR15_3 == LFSR15_comparator_value_3 )
      // LFSR15_3 = 0;
      //  LFSR5_3 = LFSR5_3++;

      //

      // LFSR5_3 = LFSR5_3 + 1;
      LFSR5_3 = LFSR5_3 + Divided_LFSR15_3 ; // increase LFSR5 counter and check how many (LFSR5_3 >= LFSR5_comparator_value_3) was skipped

      if ((ADSR_stage_3 == 1) | (LFSR5_3 >= LFSR5_comparator_value_3) ) {

        //  LFSR5_3 = 0;
        Divided_LFSR5_3 = (LFSR5_3 ) / LFSR5_comparator_value_3; //      ceo5 = (lfsr5 - previous5) / comp5 // comp5 in attack is always 1
        if (Divided_LFSR5_3 >= 1) {
          LFSR5_3 = 0;
        }
        else {

          LFSR5_3 =  LFSR5_3 - Divided_LFSR5_3 * LFSR5_comparator_value_3; //      set to zero, plus how manu uS has passed since (LFSR5_3 == LFSR5_comparator_value_3)
        }



        if (hold_zero_3 == false) {
          // 0-release finished , 1-Attack, 2-Decay, 3-Sustain, 4-Release
          switch (ADSR_stage_3) {
            case 0: // release finished, no change
              // lalalalala
              break;
            case 1: // Attack stage
              //  ADSR_volume_3 = (ADSR_volume_3 + 1) & 0xff;
              ADSR_volume_3 = (ADSR_volume_3 + Divided_LFSR15_3) ; // increase  volume value (value is unsigned 8bit, but variable is 16bit unsigned integer ), including skipped steps ;
              if (ADSR_volume_3 >= 0xff) { // Attack finished, start Decay //
                ADSR_volume_3 = 0xff - (ADSR_volume_3 - 0xff); // we are in decay stage already, calculate it's volume, depending of uS passed since (ADSR_volume_3 == 0xff)
                ADSR_stage_3 = 2; // set decay stage
                hold_zero_3 = false;
                LFSR15_comparator_value_3 = ADSR_LFSR15[ADSR_Decay_3   ]; // set counter comparator to match decay value
              }
              break;
            case 2: // Decay stage

              // 1 step of volume down
              //ADSR_volume_3 = ADSR_volume_3 - 1;
              if (ADSR_volume_3 >= Divided_LFSR5_3)        {
                ADSR_volume_3 = ADSR_volume_3 - Divided_LFSR5_3; //  keep it as positive number
              }
              else {
                ADSR_volume_3 = 0; // no negative numbers, clip it to zero
              }

              if (ADSR_volume_3 <= (( ADSR_Sustain_3 << 4) + ADSR_Sustain_3)) {
                ADSR_volume_3 = (( ADSR_Sustain_3 << 4) + ADSR_Sustain_3);
                LFSR15_comparator_value_3 = ADSR_LFSR15[ADSR_Release_3   ]; // set counter comparator to match release value
                // decay finished, time to sustain
                ADSR_stage_3 = 3;

              }
              break;
            case 3: // Sustain stage - checking for change in sustain value
              if (ADSR_volume_3 > (( ADSR_Sustain_3 << 4) + ADSR_Sustain_3)) {
                // new sustain value is smaller then old,  get back to decay.


                ADSR_stage_3 = 2; // set Decay to new value
                LFSR15_comparator_value_3 = ADSR_LFSR15[ADSR_Decay_3   ]; // set counter comparator to match decay value
              }
              break;
            case 4: // Release stage

              //  ADSR_volume_3 = (ADSR_volume_3 - 1) & 0xff;
              if (ADSR_volume_3 >= Divided_LFSR5_3)        {
                ADSR_volume_3 = ADSR_volume_3 - Divided_LFSR5_3; //  keep it as positive number
              }
              else {
                ADSR_volume_3 = 0; // no negative numbers, clip it to zero
              }
              break;

          } // ADSR_stage switch


          LFSR5_comparator_value_3 = ADSR_Volume2LFSR5[ADSR_volume_3]; // must look into table, exact values are not possible


          if (ADSR_volume_3 == 0) {
            hold_zero_3 = true;
          }

        } // not hold zero
      }  // LFSR5_comparator_value check

      //


    }  // LFSR15_comparator_value check




    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //

    // finished calculations, time to set main volume

    // WaveformDA : 12bit     ( 0 -> 0x0fff  )
    // ADSR_volume : 8bit     ( 0 -> 0x00ff  )
    // Volume_x = 20bit       ( 0 -> 0xfffff )
    //

    // set WaveformDA of undefined channel to 0
#ifndef USE_CHANNEL_1
    WaveformDA_1 = 0;
#endif
#ifndef USE_CHANNEL_2
    WaveformDA_2 = 0;
#endif
#ifndef USE_CHANNEL_3
    WaveformDA_3 = 0;
#endif



    //////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////// ANALOG ///////////////////////////
    //ok, so far all values are positive, time to go analog
    // Volume       - 20bit , but now signed (+/- 0x7ffff )


    Volume_1 = (int32_t)(WaveformDA_1 * ADSR_volume_1) - 0x80000 ;
    Volume_2 = (int32_t)(WaveformDA_2 * ADSR_volume_2) - 0x80000 ;
    Volume_3 = (int32_t)(WaveformDA_3 * ADSR_volume_3) - 0x80000 ;



    ////////////////////////////////////
    ///////////////////////////////////////////////
    /////////////////////////////////////////////////// FILTERS redirect to filtered or unfiltered output

    FILTER_Enable_switch =  SID[23]&0xF;

    switch (FILTER_Enable_switch) {
      default:
      case 0x0:
        Volume_filtered = 0;
        if (OFF3 )
        { // voice3 is not muted if passed thrue filter
          Volume_unfiltered = Volume_1 + Volume_2;
        }
        else {
          Volume_unfiltered = Volume_1 + Volume_2 + Volume_3 ;
        }
        break;
      case 0x1:
        Volume_filtered = Volume_1;
        if (OFF3 )
        { // voice3 is not muted if passed thrue filter
          Volume_unfiltered =  Volume_2;
        }
        else
        {
          Volume_unfiltered = Volume_2 + Volume_3 ;
        }
        break;
      case 0x2:
        Volume_filtered = Volume_2;
        if (OFF3 )
        { // voice3 is not muted if passed thrue filter
          Volume_unfiltered = Volume_1  ;
        }
        else
        {
          Volume_unfiltered = Volume_1 + Volume_3 ;
        }
        break;
      case 0x3:
        Volume_filtered = Volume_1 + Volume_2;
        if (OFF3 )
        { // voice3 is not muted if passed thrue filter
          Volume_unfiltered = 0 ;
        }
        else
        {
          Volume_unfiltered = Volume_3 ;
        }
        break;
      case 0x4: // voice3 is included, no matter the state of OFF3
        Volume_filtered = Volume_3;
        Volume_unfiltered = Volume_1 + Volume_2 ;
        break;
      case 0x5:
        Volume_filtered = Volume_1 + Volume_3;
        Volume_unfiltered = Volume_2 ;
        break;
      case 0x6:
        Volume_filtered = Volume_2 + Volume_3;
        Volume_unfiltered = Volume_1 ;
        break;
      case 0x7:
        Volume_filtered = Volume_1 + Volume_2 + Volume_3;
        Volume_unfiltered = 0;
        break;
    }
    Volume_filter_input = Volume_filtered;
    Volume_filter_output = Volume_filtered; // in case filters are skipped

#ifdef USE_FILTERS
    Volume_filter_input = (int32_t)(Volume_filter_input) >> 7; // lower it to 13bit

    // w0 and Q is calculated emulator's address decoder (to save few uS here)
    // reSID:
    // Maximum delta cycles for the filter to work satisfactorily under current
    // cutoff frequency and resonance constraints is approximately 8.

    delta_t = multiplier;
    delta_t_flt = FILTER_SENSITIVITY;

    while (delta_t) {
      if (delta_t < delta_t_flt) {
        delta_t_flt = delta_t;
      }
      // reSID:
      // delta_t is converted to seconds given a 1MHz clock by dividing
      // with 1 000 000. This is done in two operations to avoid integer
      // multiplication overflow.

      // reSID:
      //// Calculate filter outputs.
      //// Vhp = Vbp/Q - Vlp - Vi;
      //// dVbp = -w0*Vhp*dt;
      //// dVlp = -w0*Vbp*dt;
      w0_delta_t = ((int32_t)(w0_ceil_dt * delta_t_flt) >> 6);

      dVbp = ((int32_t)(w0_delta_t*Vhp) >> 14);
      dVlp = ((int32_t)(w0_delta_t*Vbp) >> 14);

      Vbp -= dVbp;
      Vlp -= dVlp;
      Vhp = ((int32_t)(Vbp * (Q_1024_div)) >> 10) - Vlp - Volume_filter_input; // i am not sure is this order is good. Maybe Vhp is calculated first, before Vbp and Vlp?

      delta_t -= delta_t_flt;
    }

    Volume_filter_output = 0;

    Volume_filter_output = ((SID[24]&0b10000) ? Vlp : 0) + 
                          ((SID[24]&0b100000) ? Vbp : 0) + 
                          ((SID[24]&0b1000000) ? Vhp : 0);

    Volume_filter_output = ((int32_t)(Volume_filter_output) << 7); // back to 20 bit

#endif

    // magic_number - 8bit
    // period       - 8bit
    // Volume       - 20bit

    // time to get positive about it


    Volume = ((Volume_filter_output + Volume_unfiltered) >> 2 ) + 0x80000; // keep as 20bit unsigned value, but it's divided by 4, not 3, because resonance

    if (Volume < 0) Volume = 0;
    if (Volume > 0xfffff) Volume = 0xfffff; // remove clipping (resonance sensitivity), just in case..

    main_volume_32bit = (Volume) ; //& 0xfffff ; // 20bit
    main_volume_32bit = (main_volume_32bit * 0x10); // 28bit
    main_volume_32bit = (main_volume_32bit) >> 7; // 28-12 = 16bit
    main_volume_32bit = (main_volume_32bit *  (SID[24]&0x0F)); //16+4 =20bit MASTER_VOLUME
    main_volume_32bit = (main_volume_32bit) >> 9; // 28-12 = 16bit
    main_volume = main_volume_32bit + 1; // i forgot why i added this. Maybe for minimum value for CCR1?


    ENV3 = (ADSR_volume_3) & 0xff; ; // ((Volume_3 + 0x80000) >> 12) & 0xff; // value for REG_28
    SID[25] = POTX;
    SID[26] = POTY;
    SID[27] = (WaveformDA_3 >> 4) & 0xff; //WaveformDA_3 - 12 bit
    SID[28] = ENV3;

    // btw, lot of "i hope" in this code... oh, well... :-)

    STAD4XX = 0; // let main program know that his request has been served           
}
