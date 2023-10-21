//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#define __ENVELOPE_CC__
#include "envelope.h"
#include "common.h"

// Rate counter periods are calculated from the Envelope Rates table in
// the Programmer's Reference Guide. The rate counter period is the number of
// cycles between each increment of the envelope counter.
// The rates have been verified by sampling ENV3. 
//
// The rate counter is a 16 bit register which is incremented each cycle.
// When the counter reaches a specific comparison value, the envelope counter
// is incremented (attack) or decremented (decay/release) and the
// counter is zeroed.
//
// NB! Sampling ENV3 shows that the calculated values are not exact.
// It may seem like most calculated values have been rounded (.5 is rounded
// down) and 1 has beed added to the result. A possible explanation for this
// is that the SID designers have used the calculated values directly
// as rate counter comparison values, not considering a one cycle delay to
// zero the counter. This would yield an actual period of comparison value + 1.
//
// The time of the first envelope count can not be exactly controlled, except
// possibly by resetting the chip. Because of this we cannot do cycle exact
// sampling and must devise another method to calculate the rate counter
// periods.
//
// The exact rate counter periods can be determined e.g. by counting the number
// of cycles from envelope level 1 to envelope level 129, and dividing the
// number of cycles by 128. CIA1 timer A and B in linked mode can perform
// the cycle count. This is the method used to find the rates below.
//
// To avoid the ADSR delay bug, sampling of ENV3 should be done using
// sustain = release = 0. This ensures that the attack state will not lower
// the current rate counter period.
//
// The ENV3 sampling code below yields a maximum timing error of 14 cycles.
//     lda #$01
// l1: cmp $d41c
//     bne l1
//     ...
//     lda #$ff
// l2: cmp $d41c
//     bne l2
//
// This yields a maximum error for the calculated rate period of 14/128 cycles.
// The described method is thus sufficient for exact calculation of the rate
// periods.
//

  // Lookup table to convert from attack, decay, or release value to rate
  // counter period.
const reg16 rate_counter_period[] = {
      9,  //   2ms*1.0MHz/256 =     7.81
     32,  //   8ms*1.0MHz/256 =    31.25
     63,  //  16ms*1.0MHz/256 =    62.50
     95,  //  24ms*1.0MHz/256 =    93.75
    149,  //  38ms*1.0MHz/256 =   148.44
    220,  //  56ms*1.0MHz/256 =   218.75
    267,  //  68ms*1.0MHz/256 =   265.63
    313,  //  80ms*1.0MHz/256 =   312.50
    392,  // 100ms*1.0MHz/256 =   390.63
    977,  // 250ms*1.0MHz/256 =   976.56
   1954,  // 500ms*1.0MHz/256 =  1953.13
   3126,  // 800ms*1.0MHz/256 =  3125.00
   3907,  //   1 s*1.0MHz/256 =  3906.25
  11720,  //   3 s*1.0MHz/256 = 11718.75
  19532,  //   5 s*1.0MHz/256 = 19531.25
  31251   //   8 s*1.0MHz/256 = 31250.00
};

// From the sustain levels it follows that both the low and high 4 bits of the
// envelope counter are compared to the 4-bit sustain value.
// This has been verified by sampling ENV3.
//
const reg8 sustain_level[] = {
  0x00,
  0x11,
  0x22,
  0x33,
  0x44,
  0x55,
  0x66,
  0x77,
  0x88,
  0x99,
  0xaa,
  0xbb,
  0xcc,
  0xdd,
  0xee,
  0xff,
};


// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void EnvelopeGenerator_reset(struct EnvelopeGenerator* gen)
{
  gen->envelope_counter = 0;

  gen->attack = 0;
  gen->decay = 0;
  gen->sustain = 0;
  gen->release = 0;

  gen->gate = 0;

  gen->rate_counter = 0;
  gen->exponential_counter = 0;
  gen->exponential_counter_period = 1;

  gen->state = RELEASE;
  gen->rate_period = rate_counter_period[gen->release];
  gen->hold_zero = true;
}


// For decay and release, the clock to the envelope counter is sequentially
// divided by 1, 2, 4, 8, 16, 30, 1 to create a piece-wise linear approximation
// of an exponential. The exponential counter period is loaded at the envelope
// counter values 255, 93, 54, 26, 14, 6, 0. The period can be different for the
// same envelope counter value, depending on whether the envelope has been
// rising (attack -> release) or sinking (decay/release).
//
// Since it is not possible to reset the rate counter (the test bit has no
// influence on the envelope generator whatsoever) a method must be devised to
// do cycle exact sampling of ENV3 to do the investigation. This is possible
// with knowledge of the rate period for A=0, found above.
//
// The CPU can be synchronized with ENV3 by first synchronizing with the rate
// counter by setting A=0 and wait in a carefully timed loop for the envelope
// counter _not_ to change for 9 cycles. We can then wait for a specific value
// of ENV3 with another timed loop to fully synchronize with ENV3.
//
// At the first period when an exponential counter period larger than one
// is used (decay or relase), one extra cycle is spent before the envelope is
// decremented. The envelope output is then delayed one cycle until the state
// is changed to attack. Now one cycle less will be spent before the envelope
// is incremented, and the situation is normalized.
// The delay is probably caused by the comparison with the exponential counter,
// and does not seem to affect the rate counter. This has been verified by
// timing 256 consecutive complete envelopes with A = D = R = 1, S = 0, using
// CIA1 timer A and B in linked mode. If the rate counter is not affected the
// period of each complete envelope is
// (255 + 162*1 + 39*2 + 28*4 + 12*8 + 8*16 + 6*30)*32 = 756*32 = 32352
// which corresponds exactly to the timed value divided by the number of
// complete envelopes.
// NB! This one cycle delay is not modeled.

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
FORCE_INLINE void EnvelopeGenerator_writeCONTROL_REG(struct EnvelopeGenerator* gen, reg8 control)
{
  reg8 gate_next = control & 0x01;

  // The rate counter is never reset, thus there will be a delay before the
  // envelope counter starts counting up (attack) or down (release).

  // Gate bit on: Start attack, decay, sustain.
  if (!gen->gate && gate_next) {
    gen->state = ATTACK;
    gen->rate_period = rate_counter_period[gen->attack];

    // Switching to attack state unlocks the zero freeze.
    gen->hold_zero = false;
  }
  // Gate bit off: Start release.
  else if (gen->gate && !gate_next) {
    gen->state = RELEASE;
    gen->rate_period = rate_counter_period[gen->release];
  }

  gen->gate = gate_next;
}

FORCE_INLINE void EnvelopeGenerator_writeATTACK_DECAY(struct EnvelopeGenerator* gen, reg8 attack_decay)
{
  gen->attack = (attack_decay >> 4) & 0x0f;
  gen->decay = attack_decay & 0x0f;
  if (gen->state == ATTACK) {
    gen->rate_period = rate_counter_period[gen->attack];
  }
  else if (gen->state == DECAY_SUSTAIN) {
    gen->rate_period = rate_counter_period[gen->decay];
  }
}

FORCE_INLINE void EnvelopeGenerator_writeSUSTAIN_RELEASE(struct EnvelopeGenerator* gen, reg8 sustain_release)
{
  gen->sustain = (sustain_release >> 4) & 0x0f;
  gen->release = sustain_release & 0x0f;
  if (gen->state == RELEASE) {
    gen->rate_period = rate_counter_period[gen->release];
  }
}

reg8 EnvelopeGenerator_readENV(struct EnvelopeGenerator* gen)
{
  return EnvelopeGenerator_output(gen);
}

// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__ENVELOPE_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGenerator_clock(struct EnvelopeGenerator* gen)
{
  // Check for ADSR delay bug.
  // If the rate counter comparison value is set below the current value of the
  // rate counter, the counter will continue counting up until it wraps around
  // to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
  // envelope can finally be stepped.
  // This has been verified by sampling ENV3.
  //
  gen->rate_counter+=1;
  if (gen->rate_counter & 0x8000) {
    gen->rate_counter+=1;
    gen->rate_counter &= 0x7fff;
  }

  if (gen->rate_counter != gen->rate_period) {
    return;
  }

  gen->rate_counter = 0;

  // The first envelope step in the attack state also resets the exponential
  // counter. This has been verified by sampling ENV3.
  //
  if (gen->state == ATTACK || ++gen->exponential_counter == gen->exponential_counter_period)
  {
    gen->exponential_counter = 0;

    // Check whether the envelope counter is frozen at zero.
    if (gen->hold_zero) {
      return;
    }

    switch (gen->state) {
    case ATTACK:
      // The envelope counter can flip from 0xff to 0x00 by changing state to
      // release, then to attack. The envelope counter is then frozen at
      // zero; to unlock this situation the state must be changed to release,
      // then to attack. This has been verified by sampling ENV3.
      //
      gen->envelope_counter+=1;
      gen->envelope_counter &= 0xff;
      if (gen->envelope_counter == 0xff) {
      gen->state = DECAY_SUSTAIN;
      gen->rate_period = rate_counter_period[gen->decay];
      }
      break;
    case DECAY_SUSTAIN:
      if (gen->envelope_counter != sustain_level[gen->sustain]) {
	     --gen->envelope_counter;
      }
      break;
    case RELEASE:
      // The envelope counter can flip from 0x00 to 0xff by changing state to
      // attack, then to release. The envelope counter will then continue
      // counting down in the release state.
      // This has been verified by sampling ENV3.
      // NB! The operation below requires two's complement integer.
      //
      gen->envelope_counter-=1;
      gen->envelope_counter &= 0xff;
      break;
    }
    
    // Check for change of exponential counter period.
    switch (gen->envelope_counter) {
    case 0xff:
      gen->exponential_counter_period = 1;
      break;
    case 0x5d:
      gen->exponential_counter_period = 2;
      break;
    case 0x36:
      gen->exponential_counter_period = 4;
      break;
    case 0x1a:
      gen->exponential_counter_period = 8;
      break;
    case 0x0e:
      gen->exponential_counter_period = 16;
      break;
    case 0x06:
      gen->exponential_counter_period = 30;
      break;
    case 0x00:
      gen->exponential_counter_period = 1;

      // When the envelope counter is changed to zero, it is frozen at zero.
      // This has been verified by sampling ENV3.
      gen->hold_zero = true;
      break;
    }
  }
}


// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void EnvelopeGenerator_clock_dt(struct EnvelopeGenerator* gen, cycle_count delta_t)
{
  // Check for ADSR delay bug.
  // If the rate counter comparison value is set below the current value of the
  // rate counter, the counter will continue counting up until it wraps around
  // to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
  // envelope can finally be stepped.
  // This has been verified by sampling ENV3.
  //

  // NB! This requires two's complement integer.
  int rate_step = gen->rate_period - gen->rate_counter;
  if (rate_step <= 0) {
    rate_step += 0x7fff;
  }

  while (delta_t) {
    if (delta_t < rate_step) {
      gen->rate_counter += delta_t;
      if (gen->rate_counter & 0x8000) {
	      gen->rate_counter+=1;
        gen->rate_counter &= 0x7fff;
      }
      return;
    }

    gen->rate_counter = 0;
    delta_t -= rate_step;

    // The first envelope step in the attack state also resets the exponential
    // counter. This has been verified by sampling ENV3.
    //
    if (gen->state == ATTACK	|| ++gen->exponential_counter == gen->exponential_counter_period)
    {
      gen->exponential_counter = 0;

      // Check whether the envelope counter is frozen at zero.
      if (gen->hold_zero) {
        rate_step = gen->rate_period;
        continue;
      }

      switch (gen->state) {
          case ATTACK:
            // The envelope counter can flip from 0xff to 0x00 by changing state to
            // release, then to attack. The envelope counter is then frozen at
            // zero; to unlock this situation the state must be changed to release,
            // then to attack. This has been verified by sampling ENV3.
            //
            gen->envelope_counter+=1;
            gen->envelope_counter &= 0xff;
            if (gen->envelope_counter == 0xff) {
              gen->state = DECAY_SUSTAIN;
              gen->rate_period = rate_counter_period[gen->decay];
            }
            break;
          case DECAY_SUSTAIN:
            if (gen->envelope_counter != sustain_level[gen->sustain]) {
              gen->envelope_counter-=1;
            }
            break;
          case RELEASE:
            // The envelope counter can flip from 0x00 to 0xff by changing state to
            // attack, then to release. The envelope counter will then continue
            // counting down in the release state.
            // This has been verified by sampling ENV3.
            // NB! The operation below requires two's complement integer.
            //
            gen->envelope_counter-=1;
            gen->envelope_counter &= 0xff;
          break;
      }

      // Check for change of exponential counter period.
      switch (gen->envelope_counter) {
      case 0xff:
        gen->exponential_counter_period = 1;
        break;
      case 0x5d:
        gen->exponential_counter_period = 2;
        break;
      case 0x36:
        gen->exponential_counter_period = 4;
        break;
      case 0x1a:
        gen->exponential_counter_period = 8;
        break;
      case 0x0e:
        gen->exponential_counter_period = 16;
        break;
      case 0x06:
        gen->exponential_counter_period = 30;
        break;
      case 0x00:
        gen->exponential_counter_period = 1;

        // When the envelope counter is changed to zero, it is frozen at zero.
        // This has been verified by sampling ENV3.
        gen->hold_zero = true;
        break;
      }
    }
    rate_step = gen->rate_period;
  }
}


// ----------------------------------------------------------------------------
// Read the envelope generator output.
// ----------------------------------------------------------------------------
RESID_INLINE
reg8 EnvelopeGenerator_output(struct EnvelopeGenerator* gen)
{
  return gen->envelope_counter;
}

#endif // RESID_INLINING || defined(__ENVELOPE_CC__)

