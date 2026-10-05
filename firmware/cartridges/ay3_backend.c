/*
 * Integer AY-3-8910-style backend for KungFuSID.
 *
 * The tone/noise model and logarithmic DAC curve are based on the AY/YM
 * behavior documented and implemented by Peter Sovietov's Ayumi project,
 * used under the MIT license. This backend is a new fixed-point adaptation
 * for KungFuSID's interrupt-driven mono DAC output.
 *
 * Copyright (c) Peter Sovietov
 * Copyright (c) 2026 KungFuSID contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "ay3_backend.h"

#include <string.h>

#include "sid.h"
#include "xparam_eeprom.h"

#define AY3_CHANNEL_COUNT       3U
#define AY3_CLOCK_DIVIDER       8U
#define AY3_MAX_TONE_PERIOD     4095U
#define AY3_MAX_NOISE_PERIOD    31U
#define AY3_SID_PERIOD_NUMERATOR (1UL << 20)

typedef struct
{
    uint16_t period;
    uint16_t counter;
    uint8_t tone;
    uint8_t tone_enabled;
    uint8_t noise_enabled;
    uint8_t control;
} ay3_channel_t;

typedef struct
{
    ay3_channel_t channels[AY3_CHANNEL_COUNT];
    uint16_t clock_remainder;
    uint16_t noise_counter;
    uint8_t noise_period;
    uint32_t noise_lfsr;
    int32_t filter_low;
} ay3_state_t;

static ay3_state_t ay3;

/* Integer approximation of the AY-3-8910 logarithmic 16-level DAC. */
static const uint16_t ay3_dac_level[16] =
{
       0,   10,   15,   22,   31,   47,   66,  110,
     130,  210,  299,  382,  504,  651,  825, 1024
};

static void ay3_update_noise_period(void)
{
    uint16_t shortest_period = AY3_MAX_NOISE_PERIOD;
    uint8_t channel;
    uint8_t enabled = 0;

    /* An AY has one noise generator shared by all channels. When several
     * SID noise voices are active, let the highest-pitched one drive it. */
    for (channel = 0; channel < AY3_CHANNEL_COUNT; ++channel)
    {
        if (ay3.channels[channel].noise_enabled)
        {
            uint16_t period = ay3.channels[channel].period;
            if (period < shortest_period)
                shortest_period = period;
            enabled = 1;
        }
    }

    ay3.noise_period = enabled ? (uint8_t)shortest_period : 1U;
}

static uint16_t ay3_sid_frequency_to_period(uint16_t sid_frequency)
{
    uint32_t period;

    if (sid_frequency == 0)
        return AY3_MAX_TONE_PERIOD;

    /* SID: f = reg * 1 MHz / 2^24.
     * AY:  f = 1 MHz / (16 * period).
     * Therefore period = 2^20 / SID register. */
    period = (AY3_SID_PERIOD_NUMERATOR + (sid_frequency >> 1)) /
             sid_frequency;
    if (period < 1U)
        period = 1U;
    if (period > AY3_MAX_TONE_PERIOD)
        period = AY3_MAX_TONE_PERIOD;
    return (uint16_t)period;
}

static void ay3_update_frequency(uint8_t channel)
{
    uint8_t base = (uint8_t)(channel * 7U);
    uint16_t sid_frequency =
        (uint16_t)SID[base] | ((uint16_t)SID[base + 1U] << 8);
    uint16_t period = ay3_sid_frequency_to_period(sid_frequency);

    ay3.channels[channel].period = period;
    if (ay3.channels[channel].counter >= period)
        ay3.channels[channel].counter %= period;

    ay3_update_noise_period();
}

static void ay3_update_control(uint8_t channel)
{
    uint8_t base = (uint8_t)(channel * 7U);
    uint8_t control = SID[base + 4U];
    ay3_channel_t* voice = &ay3.channels[channel];

    voice->control = control;
    /* AY has one square-wave tone source. SID triangle, saw and pulse all
     * select it; SID noise selects the shared AY noise source. */
    voice->tone_enabled = (control & 0x70U) != 0;
    voice->noise_enabled = (control & 0x80U) != 0;

    if (control & 0x08U)
    {
        voice->counter = 0;
        voice->tone = 0;
    }

    ay3_update_noise_period();
}

static uint8_t ay3_advance_tone(ay3_channel_t* voice, uint16_t ticks)
{
    uint8_t rising = 0;

    voice->counter = (uint16_t)(voice->counter + ticks);
    while (voice->counter >= voice->period)
    {
        voice->counter = (uint16_t)(voice->counter - voice->period);
        voice->tone ^= 1U;
        if (voice->tone)
            rising = 1;
    }
    return rising;
}

static uint8_t ay3_advance_noise(uint16_t ticks)
{
    uint16_t threshold = (uint16_t)ay3.noise_period << 1;

    ay3.noise_counter = (uint16_t)(ay3.noise_counter + ticks);
    while (ay3.noise_counter >= threshold)
    {
        uint32_t feedback = (ay3.noise_lfsr ^
                             (ay3.noise_lfsr >> 3)) & 1U;
        ay3.noise_counter = (uint16_t)(ay3.noise_counter - threshold);
        ay3.noise_lfsr = (ay3.noise_lfsr >> 1) | (feedback << 16);
    }
    return (uint8_t)(ay3.noise_lfsr & 1U);
}

static int32_t ay3_voice_sample(uint8_t channel, uint8_t envelope,
                                uint8_t noise)
{
    static const uint8_t ring_source[AY3_CHANNEL_COUNT] = { 2, 0, 1 };
    ay3_channel_t* voice = &ay3.channels[channel];
    uint8_t tone = voice->tone;
    uint8_t output;
    uint16_t level;

    if (!voice->tone_enabled && !voice->noise_enabled)
        return 0;

    if ((voice->control & 0x04U) && voice->tone_enabled)
        tone ^= ay3.channels[ring_source[channel]].tone;

    output = (uint8_t)((tone || !voice->tone_enabled) &&
                       (noise || !voice->noise_enabled));
    level = ay3_dac_level[envelope >> 4];
    return output ? (int32_t)level : -(int32_t)level;
}

static int32_t ay3_filter(int32_t input)
{
    uint16_t coefficient = (uint16_t)(16U + (FILTER_HiLo >> 3));
    int32_t previous_low = ay3.filter_low;
    int32_t high;
    int32_t band;
    int32_t output = 0;

    if (coefficient > 255U)
        coefficient = 255U;

    ay3.filter_low += ((input - ay3.filter_low) * coefficient) >> 8;
    high = input - ay3.filter_low;
    band = (ay3.filter_low - previous_low) * 4;
    /* Keep SID filter routing, but deliberately use a coarse one-pole model.
     * Multiple selected modes add together and resonance emphasizes the
     * unsmoothed edge, producing the intended harder PSG character. */
    if (SID[24] & 0x10U)
        output += ay3.filter_low;
    if (SID[24] & 0x20U)
        output += band;
    if (SID[24] & 0x40U)
        output += high;
    if (SID[24] & 0x70U)
        output += (high * FILTER_Resonance) >> 4;

    return output & ~7;
}

void ay3_backend_reset(void)
{
    uint8_t channel;

    reset_SID();
    memset(&ay3, 0, sizeof(ay3));
    ay3.noise_lfsr = 1U;
    ay3.noise_period = 1U;
    for (channel = 0; channel < AY3_CHANNEL_COUNT; ++channel)
        ay3.channels[channel].period = AY3_MAX_TONE_PERIOD;
}

void ay3_backend_write(uint8_t address, uint8_t value)
{
    uint8_t channel;
    uint8_t voice_register;

    /* Retain normal SID register/readback and ADSR state handling, then map
     * oscillator-related writes into AY periods and mixer controls. */
    setreg(address, value);
    if (address >= 21U)
        return;

    channel = address / 7U;
    voice_register = address - channel * 7U;
    if (voice_register <= 1U)
        ay3_update_frequency(channel);
    else if (voice_register == 4U)
        ay3_update_control(channel);
}

void ay3_backend_cycle(void)
{
    static const uint8_t sync_source[AY3_CHANNEL_COUNT] = { 2, 0, 1 };
    uint8_t rising[AY3_CHANNEL_COUNT];
    uint8_t noise;
    uint8_t channel;
    uint16_t ticks;
    int32_t filtered = 0;
    int32_t unfiltered = 0;
    int32_t mixed;
    int32_t scaled;
    uint8_t envelope[AY3_CHANNEL_COUNT];

    ay3.clock_remainder = (uint16_t)(ay3.clock_remainder + SID_MULTIPLIER);
    ticks = ay3.clock_remainder / AY3_CLOCK_DIVIDER;
    ay3.clock_remainder %= AY3_CLOCK_DIVIDER;

    for (channel = 0; channel < AY3_CHANNEL_COUNT; ++channel)
        rising[channel] = ay3_advance_tone(&ay3.channels[channel], ticks);

    /* SID oscillator sync resets a voice when its source oscillator rises. */
    for (channel = 0; channel < AY3_CHANNEL_COUNT; ++channel)
    {
        if ((ay3.channels[channel].control & 0x02U) &&
            rising[sync_source[channel]])
        {
            ay3.channels[channel].counter = 0;
            ay3.channels[channel].tone = 0;
        }
    }

    noise = ay3_advance_noise(ticks);

    EnvelopeGenerator_clock_dt(&gen1, SID_MULTIPLIER);
    EnvelopeGenerator_clock_dt(&gen2, SID_MULTIPLIER);
    EnvelopeGenerator_clock_dt(&gen3, SID_MULTIPLIER);
    envelope[0] = EnvelopeGenerator_output(&gen1);
    envelope[1] = EnvelopeGenerator_output(&gen2);
    envelope[2] = EnvelopeGenerator_output(&gen3);
    ADSR_volume_1 = envelope[0];
    ADSR_volume_2 = envelope[1];
    ADSR_volume_3 = envelope[2];

    for (channel = 0; channel < AY3_CHANNEL_COUNT; ++channel)
    {
        int32_t sample = ay3_voice_sample(channel, envelope[channel], noise);
        uint8_t routed_to_filter = SID[23] & (1U << channel);

        if (channel == 2U && (SID[24] & 0x80U) && !routed_to_filter)
            sample = 0;

        if (routed_to_filter)
            filtered += sample;
        else
            unfiltered += sample;
    }

    mixed = unfiltered + ay3_filter(filtered);
    scaled = (mixed * (SID[24] & 0x0FU)) / 30;
    scaled = (scaled * sid_output_gain_percent) / 100;
    scaled += 2048;
    if (scaled < 0)
        scaled = 0;
    else if (scaled > 4095)
        scaled = 4095;

    main_volume = (uint16_t)scaled;
    main_volume_32bit = (uint32_t)scaled;
    SID[27] = ay3.channels[2].tone ? 0xFFU : 0x00U;
    SID[28] = envelope[2];
}
