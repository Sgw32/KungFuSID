#include "setup.h"
#include "irq.h"
#include "sid.h"
#include "envelope.c"

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

FORCE_INLINE uint32_t sid_noise_advance(uint32_t state, uint32_t steps, uint8_t *last_bit)
{
#if defined(__arm__) || defined(__thumb__)
  uint32_t feedback = *last_bit;
  uint32_t scratch;

  __asm volatile (
    "cmp %[steps], #0\n"
    "beq 2f\n"
    "1:\n"
    "lsr %[feedback], %[state], #22\n"
    "lsr %[scratch], %[state], #17\n"
    "eor %[feedback], %[feedback], %[scratch]\n"
    "ands %[feedback], %[feedback], #1\n"
    "lsl %[state], %[state], #1\n"
    "orr %[state], %[state], %[feedback]\n"
    "subs %[steps], %[steps], #1\n"
    "bne 1b\n"
    "2:\n"
    : [state] "+r" (state),
      [steps] "+r" (steps),
      [feedback] "+r" (feedback),
      [scratch] "=&r" (scratch)
    :
    : "cc"
  );

  *last_bit = (uint8_t)feedback;
  return state;
#else
  uint32_t feedback = *last_bit;

  while (steps--) {
    feedback = (((state >> 22) ^ (state >> 17)) & 0x1);
    state = (state << 1) | feedback;
  }

  *last_bit = (uint8_t)feedback;
  return state;
#endif
}

FORCE_INLINE void sid_filter_iteration(int32_t *Vhp_ptr,
                                       int32_t *Vbp_ptr,
                                       int32_t *Vlp_ptr,
                                       int32_t *w0_delta_t_ptr,
                                       int32_t *dVbp_ptr,
                                       int32_t *dVlp_ptr,
                                       int32_t w0_ceil_dt_val,
                                       int delta_t_step,
                                       int32_t Q_1024_div_val,
                                       int32_t volume_input)
{
#if defined(__arm__) || defined(__thumb__)
  int32_t Vhp_val = *Vhp_ptr;
  int32_t Vbp_val = *Vbp_ptr;
  int32_t Vlp_val = *Vlp_ptr;
  int32_t w0_delta_val = *w0_delta_t_ptr;
  int32_t dVbp_val = *dVbp_ptr;
  int32_t dVlp_val = *dVlp_ptr;
  int32_t scratch;

  __asm volatile (
    "mul %[w0_delta], %[w0_const], %[step]\n"
    "asr %[w0_delta], %[w0_delta], #6\n"
    "mul %[dVbp], %[w0_delta], %[Vhp]\n"
    "asr %[dVbp], %[dVbp], #14\n"
    "sub %[Vbp], %[Vbp], %[dVbp]\n"
    "mul %[dVlp], %[w0_delta], %[Vlp]\n"
    "asr %[dVlp], %[dVlp], #14\n"
    "sub %[Vlp], %[Vlp], %[dVlp]\n"
    "mul %[scratch], %[Vbp], %[Qdiv]\n"
    "asr %[scratch], %[scratch], #10\n"
    "sub %[scratch], %[scratch], %[Vlp]\n"
    "sub %[scratch], %[scratch], %[Vin]\n"
    "mov %[Vhp], %[scratch]\n"
    : [Vbp] "+r" (Vbp_val),
      [Vlp] "+r" (Vlp_val),
      [Vhp] "+r" (Vhp_val),
      [w0_delta] "+r" (w0_delta_val),
      [dVbp] "+r" (dVbp_val),
      [dVlp] "+r" (dVlp_val),
      [scratch] "=&r" (scratch)
    : [w0_const] "r" (w0_ceil_dt_val),
      [step] "r" (delta_t_step),
      [Qdiv] "r" (Q_1024_div_val),
      [Vin] "r" (volume_input)
    : "cc"
  );

  *Vhp_ptr = Vhp_val;
  *Vbp_ptr = Vbp_val;
  *Vlp_ptr = Vlp_val;
  *w0_delta_t_ptr = w0_delta_val;
  *dVbp_ptr = dVbp_val;
  *dVlp_ptr = dVlp_val;
#else
  int32_t w0_delta_val = (int32_t)(w0_ceil_dt_val * delta_t_step);
  w0_delta_val >>= 6;
  int32_t dVbp_val = (int32_t)(w0_delta_val * (*Vhp_ptr));
  dVbp_val >>= 14;
  int32_t dVlp_val = (int32_t)(w0_delta_val * (*Vbp_ptr));
  dVlp_val >>= 14;

  *Vbp_ptr -= dVbp_val;
  *Vlp_ptr -= dVlp_val;
  int32_t filtered = (int32_t)((*Vbp_ptr) * Q_1024_div_val);
  filtered >>= 10;
  *Vhp_ptr = filtered - (*Vlp_ptr) - volume_input;

  *w0_delta_t_ptr = w0_delta_val;
  *dVbp_ptr = dVbp_val;
  *dVlp_ptr = dVlp_val;
#endif
}

/**
 * @brief Apply oscillator sync resets and triangle/ring modulation MSB tweaks.
 */
FORCE_INLINE void sid_update_sync_and_ringmod(void)
{
#if defined(__arm__) || defined(__thumb__)
  uint32_t osc1 = OSC_1;
  uint32_t osc2 = OSC_2;
  uint32_t osc3 = OSC_3;
  uint32_t msb1 = OSC_MSB_1;
  uint32_t msb2 = OSC_MSB_2;
  uint32_t msb3 = OSC_MSB_3;
  const uint32_t sid4 = SID[4];
  const uint32_t sid11 = SID[11];
  const uint32_t sid18 = SID[18];
  const uint32_t rise1 = MSB_Rising_1;
  const uint32_t rise2 = MSB_Rising_2;
  const uint32_t rise3 = MSB_Rising_3;
  const uint32_t mask = 0x7fffff;

  __asm volatile (
    "tst %[sid4], #0x2\n"
    "beq 0f\n"
    "tst %[rise3], #0x2\n"
    "beq 0f\n"
    "and %[osc1], %[osc1], %[mask]\n"
    "0:\n"
    "tst %[sid11], #0x2\n"
    "beq 1f\n"
    "tst %[rise1], #0x2\n"
    "beq 1f\n"
    "and %[osc2], %[osc2], %[mask]\n"
    "1:\n"
    "tst %[sid18], #0x2\n"
    "beq 2f\n"
    "tst %[rise2], #0x2\n"
    "beq 2f\n"
    "and %[osc3], %[osc3], %[mask]\n"
    "2:\n"
    "tst %[sid4], #0x10\n"
    "beq 3f\n"
    "tst %[sid4], #0x4\n"
    "beq 3f\n"
    "eor %[msb1], %[msb1], %[msb3]\n"
    "and %[msb1], %[msb1], #1\n"
    "3:\n"
    "tst %[sid11], #0x10\n"
    "beq 4f\n"
    "tst %[sid11], #0x4\n"
    "beq 4f\n"
    "eor %[msb2], %[msb2], %[msb1]\n"
    "and %[msb2], %[msb2], #1\n"
    "4:\n"
    "tst %[sid18], #0x10\n"
    "beq 5f\n"
    "tst %[sid18], #0x4\n"
    "beq 5f\n"
    "eor %[msb3], %[msb3], %[msb2]\n"
    "and %[msb3], %[msb3], #1\n"
    "5:\n"
    : [osc1] "+r" (osc1),
      [osc2] "+r" (osc2),
      [osc3] "+r" (osc3),
      [msb1] "+r" (msb1),
      [msb2] "+r" (msb2),
      [msb3] "+r" (msb3)
    : [sid4] "r" (sid4),
      [sid11] "r" (sid11),
      [sid18] "r" (sid18),
      [rise1] "r" (rise1),
      [rise2] "r" (rise2),
      [rise3] "r" (rise3),
      [mask] "r" (mask)
    : "cc"
  );

  OSC_1 = osc1;
  OSC_2 = osc2;
  OSC_3 = osc3;
  OSC_MSB_1 = (uint8_t)(msb1 & 0x1);
  OSC_MSB_2 = (uint8_t)(msb2 & 0x1);
  OSC_MSB_3 = (uint8_t)(msb3 & 0x1);
#else
  // SYNC (bit 0b10)
  if ((SID[4] & 0b10 ) & MSB_Rising_3) OSC_1 = OSC_1 & 0x7fffff;
  if ((SID[11] & 0b10 ) & MSB_Rising_1) OSC_2 = OSC_2 & 0x7fffff;
  if ((SID[18] & 0b10 ) & MSB_Rising_2) OSC_3 = OSC_3 & 0x7fffff;

  // Triangle and ringmod
  if ((SID[4] & 0b10100) == 0b10100) OSC_MSB_1 = OSC_MSB_1 ^ OSC_MSB_3; // this one took really long time to figure it out. I tought OSC_MSB_1 =  OSC_MSB_3 and everything was wacky with ring modulation
  if ((SID[11] & 0b10100) == 0b10100) OSC_MSB_2 = OSC_MSB_2 ^ OSC_MSB_1; // TODO: see if it's exact on high frequencies
  if ((SID[18] & 0b10100) == 0b10100) OSC_MSB_3 = OSC_MSB_3 ^ OSC_MSB_2; // TODO: see what's faster, here or in triangle voice check
#endif
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
    pseudorandom_1 = sid_noise_advance(pseudorandom_1, OSC_bit19_1, &bit_0_1);
    OSC_noise_1 = OSC_noise_1 - (OSC_bit19_1 << 19); // * 0x080000); // no reset, keep lower 18bit


    // noise_2
    OSC_noise_2 = OSC_noise_2 + multiplier * OSC_2_HiLo; // noise counter (
    OSC_bit19_2 = OSC_noise_2 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    pseudorandom_2 = sid_noise_advance(pseudorandom_2, OSC_bit19_2, &bit_0_2);
    OSC_noise_2 = OSC_noise_2 - (OSC_bit19_2 << 19) ; // * 0x080000); // no reset, keep lower 18bits

    // noise_3
    OSC_noise_3 = OSC_noise_3 + multiplier * OSC_3_HiLo; // noise counter (
    OSC_bit19_3 = OSC_noise_3 >> 19 ; // / 0x080000;// calculate how many missing rising edges of bit_19 since last irq
    pseudorandom_3 = sid_noise_advance(pseudorandom_3, OSC_bit19_3, &bit_0_3);
    OSC_noise_3 = OSC_noise_3 - (OSC_bit19_3 << 19 ); //  * 0x080000); // no reset, keep lower 18bit


    if (OSC_1 >= 0x800000)     OSC_MSB_1 = 1; else OSC_MSB_1 = 0;
    if ( (!OSC_MSB_Previous_1) & (OSC_MSB_1)) MSB_Rising_1 = 0b10; else  MSB_Rising_1 = 0;

    if (OSC_2 >= 0x800000)     OSC_MSB_2 = 1; else OSC_MSB_2 = 0;
    if ( (!OSC_MSB_Previous_2) & (OSC_MSB_2)) MSB_Rising_2 = 0b10; else  MSB_Rising_2 = 0;

    if (OSC_3 >= 0x800000)     OSC_MSB_3 = 1; else OSC_MSB_3 = 0;
    if ( (!OSC_MSB_Previous_3) & (OSC_MSB_3)) MSB_Rising_3 = 0b10; else MSB_Rising_3 = 0;


    // SYNC (bit 0b10) along with triangle and ring modulation interactions
    sid_update_sync_and_ringmod();

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

    while (delta_t) {
      if (delta_t < delta_t_flt) {
        delta_t_flt = delta_t;
      }

      // reSID filter integrator core handled inside sid_filter_iteration()
      sid_filter_iteration(&Vhp,
                           &Vbp,
                           &Vlp,
                           &w0_delta_t,
                           &dVbp,
                           &dVlp,
                           w0_ceil_dt,
                           delta_t_flt,
                           Q_1024_div,
                           Volume_filter_input);

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


    Volume = ((Volume_filter_output + Volume_unfiltered) >> 3 ) + 0x80000; // keep as 20bit unsigned value, but it's divided by 4, not 3, because resonance

    if (Volume < 0) Volume = 0;
    if (Volume > 0xfffff) Volume = 0xfffff; // remove clipping (resonance sensitivity), just in case..

    main_volume_32bit = (Volume) ; //& 0xfffff ; // 20bit
    main_volume_32bit = (main_volume_32bit) >> 3; // 28-12 = 16bit
    main_volume_32bit = (main_volume_32bit *  (SID[24]&0x0F)); //16+4 =20bit MASTER_VOLUME
    main_volume_32bit = (main_volume_32bit) >> 9; // 28-12 = 16bit
    main_volume = main_volume_32bit;


    ENV3 = (ADSR_volume_3) & 0xff; // ((Volume_3 + 0x80000) >> 12) & 0xff; // value for REG_28
    SID[25] = POTX;
    SID[26] = POTY;
    SID[27] = (WaveformDA_3 >> 4) & 0xff; //WaveformDA_3 - 12 bit
    SID[28] = ENV3;
}
