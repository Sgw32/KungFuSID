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

#ifndef __ENVELOPE_H__
#define __ENVELOPE_H__

#include "siddefs.h"
#include <stdbool.h>

// ----------------------------------------------------------------------------
// A 15 bit counter is used to implement the envelope rates, in effect
// dividing the clock to the envelope counter by the currently selected rate
// period.
// In addition, another counter is used to implement the exponential envelope
// decay, in effect further dividing the clock to the envelope counter.
// The period of this counter is set to 1, 2, 4, 8, 16, 30 at the envelope
// counter values 255, 93, 54, 26, 14, 6, respectively.
// ----------------------------------------------------------------------------
typedef enum { ATTACK, DECAY_SUSTAIN, RELEASE } State;


struct EnvelopeGenerator
{
  reg16 rate_counter;
  reg16 rate_period;
  reg8 exponential_counter;
  reg8 exponential_counter_period;
  reg8 envelope_counter;
  bool hold_zero;
  reg4 attack;
  reg4 decay;
  reg4 sustain;
  reg4 release;
  reg8 gate;
  State state;
};


void EnvelopeGenerator_clock(struct EnvelopeGenerator* gen);
void EnvelopeGenerator_clock_dt(struct EnvelopeGenerator* gen,cycle_count delta_t);
void EnvelopeGenerator_reset(struct EnvelopeGenerator* gen);

void EnvelopeGenerator_writeCONTROL_REG(struct EnvelopeGenerator* gen,reg8 data);
void EnvelopeGenerator_writeATTACK_DECAY(struct EnvelopeGenerator* gen,reg8 data);
void EnvelopeGenerator_writeSUSTAIN_RELEASE(struct EnvelopeGenerator* gen,reg8 data);
reg8 EnvelopeGenerator_readENV(struct EnvelopeGenerator* gen);

  // 8-bit envelope output.
reg8 EnvelopeGenerator_output(struct EnvelopeGenerator* gen);

#endif // not __ENVELOPE_H__
