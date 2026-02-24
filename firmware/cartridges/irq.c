#include "setup.h"
#include "irq.h"
#include "sid.h"
#include "envelope.c"
#include "pot.c"

static struct EnvelopeGenerator gen1;
static struct EnvelopeGenerator gen2;
static struct EnvelopeGenerator gen3;

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
    SID[addr] = value;

    switch (addr) {
      case 0:
        OSC_1_HiLo = ((SID[0] & 0xff) + ( (SID[1] & 0xff) << 8) ); 
        break;
      case 1:
        OSC_1_HiLo = ((SID[0] & 0xff) + ( (SID[1] & 0xff) << 8)); 
        break;
      case 2:
        PW_HiLo_voice_1 = SID[2] + (((SID[3] & 0x0f) << 8 ));
        break;
      case 3:
        PW_HiLo_voice_1 = SID[2] + (( (SID[3] & 0x0f) << 8 ));
        break;
      case 4:
        EnvelopeGenerator_writeCONTROL_REG(&gen1,SID[4]);
        break;
      case 5:
        EnvelopeGenerator_writeATTACK_DECAY(&gen1,SID[5]);
        break;
      case 6:
        EnvelopeGenerator_writeSUSTAIN_RELEASE(&gen1,SID[6]);
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
        EnvelopeGenerator_writeCONTROL_REG(&gen2,SID[11]);
        break;
      case 12:
        EnvelopeGenerator_writeATTACK_DECAY(&gen2,SID[12]);
        break;
      case 13:
        EnvelopeGenerator_writeSUSTAIN_RELEASE(&gen2,SID[13]);
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
        EnvelopeGenerator_writeCONTROL_REG(&gen3,SID[18]);
        break;
      case 19:
        EnvelopeGenerator_writeATTACK_DECAY(&gen3,SID[19]);
        break;
      case 20:
        EnvelopeGenerator_writeSUSTAIN_RELEASE(&gen3,SID[20]);
        break;
      case 21:
        FILTER_HiLo = (SID[21] & 0x07) + ( SID[22] << 3); // 11bit // TODO

        //set w0
        // from reSID
        // Multiply with 1.048576 to facilitate division by 1 000 000 by right-
        // shifting 20 times (2 ^ 20 = 1048576).
        // w0 = static_cast<sound_sample>(2*pi*f0[fc]*1.048576);
        //Calculations replaced with LUT
        w0_ceil_dt = w0_LUT[FILTER_HiLo];

        break;
      case 22:
        FILTER_HiLo = (SID[21] & 0x07) + (SID[22] << 3); // 11bit // TODO

        //set w0
        // Multiply with 1.048576 to facilitate division by 1 000 000 by right-
        // shifting 20 times (2 ^ 20 = 1048576).
        // w0 = static_cast<sound_sample>(2*pi*f0[fc]*1.048576);
        
        w0_ceil_dt = w0_LUT[FILTER_HiLo];
        break;
      case 23:
        //Code is replaced by lookup table
        FILTER_Resonance = ( (SID[23] >> 4 ) & 0x0f); // 4bit // TODO
        
        // // The coefficient 1024 is dispensed of later by right-shifting 10 times
        // // (2 ^ 10 = 1024).
        // // _1024_div_Q = static_cast<sound_sample>(1024.0/(0.707 + 1.0*res/0x0f));
        // Q_1024_div = (1024.0 / (0.707 + 1.0 * (FILTER_Resonance) / 15.0));
        Q_1024_div = Q_1024_LookupTable[FILTER_Resonance];
        break;
      case 24:
        OFF3 =  ( (SID[24] >> 7 ) & 1) ; // on/off; //
        break;
      default:
        break;
    }
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
  // channel 2

  OSC_2_HiLo            = 0;              // 0-65535      // 
  PW_HiLo_voice_2       = 0;              // 0-4095       // 
  // channel 3
  OSC_3_HiLo            = 0;              // 0-65535      // 
  PW_HiLo_voice_3       = 0;              // 0-4095       // 

  // other registers
  FILTER_HiLo           = 0;              // 0-2047       // 
  FILTER_Resonance      = 0;              // 0-15         // 
  OFF3                  = 0;              // true/false   // 
  memset(SID, 0, sizeof(SID));
  EnvelopeGenerator_reset(&gen1);
  EnvelopeGenerator_reset(&gen2);
  EnvelopeGenerator_reset(&gen3);
}

// Optimized 23-bit LFSR update used for noise generation. Implemented in
// ARM assembly to reduce loop overhead inside the SID interrupt routine.
FORCE_INLINE void lfsr_update(uint32_t *state, uint32_t count)
{
    if (!count)
    {
        return;
    }

    uint32_t tmp;
    __asm__ volatile (
        "1:\n"
        "lsr    %[t], %[s], #22\n"     /* bit 22 */
        "eor    %[t], %[t], %[s], lsr #17\n"  /* bit 17 */
        "ands   %[t], %[t], #1\n"       /* feedback bit */
        "lsl    %[s], %[s], #1\n"       /* shift */
        "orr    %[s], %[s], %[t]\n"     /* insert new bit */
        "subs   %[c], %[c], #1\n"       /* decrement count */
        "bne    1b\n"
        : [s] "+r" (*state), [c] "+r" (count), [t] "=&r" (tmp)
        :
        : "cc"
    );
}

// Optimized version of the filter integrator loop. The original C version
// performs multiple multiplications and branches for each iteration. By using
// ARM assembly we reduce register spills and conditional logic overhead.
FORCE_INLINE void filter_update_asm(uint32_t delta)
{
    uint32_t dt_flt = FILTER_SENSITIVITY;
    int32_t w0dt, dvbp, dvlp;

    __asm__ volatile (
        "cmp    %[d], #0\n"            /* anything to do?    */
        "beq    2f\n"
        "1:\n"
        "cmp    %[d], %[f]\n"          /* clamp dt_flt       */
        "it     lo\n"                  /* <-- required in Thumb */
        "movlo  %[f], %[d]\n"
        "mul    %[w], %[wc], %[f]\n"   /* w0_ceil_dt * dt_flt */
        "asr    %[w], %[w], #6\n"      /* >> 6               */
        "mul    %[b], %[w], %[vh]\n"   /* dVbp = w0dt*Vhp    */
        "asr    %[b], %[b], #14\n"
        "mul    %[l], %[w], %[vb]\n"   /* dVlp = w0dt*Vbp    */
        "asr    %[l], %[l], #14\n"
        "sub    %[vb], %[vb], %[b]\n"  /* Vbp -= dVbp        */
        "sub    %[vl], %[vl], %[l]\n"  /* Vlp -= dVlp        */
        "mul    %[vh], %[vb], %[qd]\n" /* Vbp*Q_1024_div     */
        "asr    %[vh], %[vh], #10\n"
        "sub    %[vh], %[vh], %[vl]\n" /* -Vlp               */
        "sub    %[vh], %[vh], %[in]\n" /* -Volume_input      */
        "subs   %[d], %[d], %[f]\n"   /* delta -= dt_flt    */
        "bne    1b\n"
        "2:\n"
        : [d] "+r" (delta), [f] "+r" (dt_flt),
          [vb] "+r" (Vbp), [vl] "+r" (Vlp), [vh] "+r" (Vhp),
          [w] "=&r" (w0dt), [b] "=&r" (dvbp), [l] "=&r" (dvlp)
        : [wc] "r" (w0_ceil_dt), [qd] "r" (Q_1024_div),
          [in] "r" (Volume_filter_input)
        : "cc"
    );
}

/**
 * @brief Main emulator function which outputs to DAC
 * 
 */
FORCE_INLINE void SID_emulator ()
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
    lfsr_update(&pseudorandom_1, OSC_bit19_1);
    OSC_noise_1 = OSC_noise_1 - (OSC_bit19_1 << 19); // * 0x080000); // no reset, keep lower 18bit


    // noise_2
    OSC_noise_2 = OSC_noise_2 + multiplier * OSC_2_HiLo; // noise counter (
    OSC_bit19_2 = OSC_noise_2 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    lfsr_update(&pseudorandom_2, OSC_bit19_2);
    OSC_noise_2 = OSC_noise_2 - (OSC_bit19_2 << 19) ; // * 0x080000); // no reset, keep lower 18bits

    // noise_3
    OSC_noise_3 = OSC_noise_3 + multiplier * OSC_3_HiLo; // noise counter (
    OSC_bit19_3 = OSC_noise_3 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    lfsr_update(&pseudorandom_3, OSC_bit19_3);
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
    if ( (SID[4]&0b10100)==0b10100) OSC_MSB_1 = OSC_MSB_1 ^ OSC_MSB_3; // this one took really long time to figure it out. I tought OSC_MSB_1 =  OSC_MSB_3 and everything was wacky with ring modulation
    if ( (SID[11]&0b10100)==0b10100) OSC_MSB_2 = OSC_MSB_2 ^ OSC_MSB_1; // TODO: see if it's exact on high frequencies
    if ( (SID[18]&0b10100)==0b10100) OSC_MSB_3 = OSC_MSB_3 ^ OSC_MSB_2; // TODO: see what's faster, here or in triangle voice check

    waveform_switch_1 = SID[4]&0xF0;
    waveform_switch_2 = SID[11]&0xF0;
    waveform_switch_3 = SID[18]&0xF0;

    temp11 = (OSC_1 >> 12); // upper 12 bit of OSC_1 calculate once now
    B2047_MSB = OSC_MSB_1 ? B2047 : 0;

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
        WaveformDA_triangle_1 = ((  B2047_MSB ^ (temp11 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
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
    B2047_MSB = OSC_MSB_2 ? B2047 : 0;

    switch (waveform_switch_2) {
      case 0b00000000:
        WaveformDA_2 = 0;
        break;
      case 0b00010000:
        WaveformDA_triangle_2 = ((  B2047_MSB ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_2 = WaveformDA_triangle_2;
        break;
      case 0b00100000:
        WaveformDA_sawtooth_2 = temp12;
        WaveformDA_2 = WaveformDA_sawtooth_2;
        break;
      case 0b00110000:
        WaveformDA_triangle_2 = ((  B2047_MSB ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_2 = temp12;
        WaveformDA_2 = AND_mask[(WaveformDA_triangle_2 & WaveformDA_sawtooth_2)] << 4;
        break;
      case 0b01000000:
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = WaveformDA_pulse_2;
        break;
      case 0b01010000:
        WaveformDA_triangle_2 = ((  B2047_MSB ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = AND_mask[WaveformDA_triangle_2 & WaveformDA_pulse_2] << 4;
        break;
      case 0b01100000:
        WaveformDA_sawtooth_2 = temp12;
        if (temp12 >= PW_HiLo_voice_2 )  WaveformDA_pulse_2 = B4095; else WaveformDA_pulse_2 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_2 = AND_mask[WaveformDA_sawtooth_2 & WaveformDA_pulse_2] << 4;
        break;
      case 0b01110000:
        WaveformDA_triangle_2 = ((  B2047_MSB ^ (temp12 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
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
    B2047_MSB = OSC_MSB_3 ? B2047 : 0;

    switch (waveform_switch_3) {
      case 0b00000000:
        WaveformDA_3 = 0;
        break;
      case 0b00010000:
        WaveformDA_triangle_3 = ((  B2047_MSB ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_3 = WaveformDA_triangle_3;
        break;
      case 0b00100000:
        WaveformDA_sawtooth_3 = temp13;
        WaveformDA_3 = WaveformDA_sawtooth_3;
        break;
      case 0b00110000:
        WaveformDA_triangle_3 = ((  B2047_MSB ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        WaveformDA_sawtooth_3 = temp13;
        WaveformDA_3 = AND_mask[(WaveformDA_triangle_3 & WaveformDA_sawtooth_3)] << 4;
        break;
      case 0b01000000:
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = WaveformDA_pulse_3;
        break;
      case 0b01010000:
        WaveformDA_triangle_3 = ((  B2047_MSB ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = AND_mask[WaveformDA_triangle_3 & WaveformDA_pulse_3] << 4;
        break;
      case 0b01100000:
        WaveformDA_sawtooth_3 = temp13;
        if (temp13 >= PW_HiLo_voice_3 )  WaveformDA_pulse_3 = B4095; else WaveformDA_pulse_3 = 0;// if upper 12bits oscilator1 is greater then value in d401/d402, then it's zero volume, else it's full
        WaveformDA_3 = AND_mask[WaveformDA_sawtooth_3 & WaveformDA_pulse_3] << 4;
        break;
      case 0b01110000:
        WaveformDA_triangle_3 = ((  B2047_MSB ^ (temp13 & B2047)) << 1) ; // (2047 or 0) xor (remaining 11 bits) and left-shifted
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
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Increase LFSR15 counter for ADSR (scaled to match)

    EnvelopeGenerator_clock_dt(&gen1,multiplier);
    ADSR_volume_1 = EnvelopeGenerator_output(&gen1);
    EnvelopeGenerator_clock_dt(&gen2,multiplier);
    ADSR_volume_2 = EnvelopeGenerator_output(&gen2);
    EnvelopeGenerator_clock_dt(&gen3,multiplier);
    ADSR_volume_3 = EnvelopeGenerator_output(&gen3);
    
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

    filter_update_asm(delta_t);

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


    Volume = ((Volume_filter_output + Volume_unfiltered) >> 3 ) + 0x80000; // keep as 20bit unsigned value, but it's divided by 4, not 3, because resonance

    if (Volume < 0) Volume = 0;
    if (Volume > 0xfffff) Volume = 0xfffff; // remove clipping (resonance sensitivity), just in case..

    main_volume_32bit = (Volume) ; //& 0xfffff ; // 20bit
    main_volume_32bit = (main_volume_32bit) >> 3; // 28-12 = 16bit
    main_volume_32bit = (main_volume_32bit *  (SID[24]&0x0F)); //16+4 =20bit MASTER_VOLUME
    main_volume_32bit = (main_volume_32bit) >> 9; // 28-12 = 16bit
    main_volume = main_volume_32bit;


    ENV3 = (ADSR_volume_3) & 0xff; // ((Volume_3 + 0x80000) >> 12) & 0xff; // value for REG_28
    pot_process();
    SID[25] = POTX;
    SID[26] = POTY;
    SID[27] = (WaveformDA_3 >> 4) & 0xff; //WaveformDA_3 - 12 bit
    SID[28] = ENV3;
}
