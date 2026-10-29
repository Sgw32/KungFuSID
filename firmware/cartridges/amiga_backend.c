#include "amiga_backend.h"

#include <string.h>

#include "amiga_sample_bank.h"
#include "sid.h"

#define AMIGA_VOICES 3U

typedef struct
{
    const amiga_bank_entry_t* entry;
    const int8_t* pcm;
    uint32_t position_q16;
    uint32_t increment_q16;
    uint32_t oscillator;
    uint8_t control;
    uint8_t gate;
    uint8_t active;
} amiga_voice_t;

static amiga_voice_t amiga_voice[AMIGA_VOICES];
static int32_t amiga_filter_low;

static uint16_t amiga_frequency(uint8_t voice)
{
    uint8_t base = (uint8_t)(voice * 7U);
    return (uint16_t)SID[base] | ((uint16_t)SID[base + 1U] << 8);
}

static uint8_t amiga_classify(uint8_t voice)
{
    uint8_t base = (uint8_t)(voice * 7U);
    uint16_t freq = amiga_frequency(voice);
    uint16_t pulse = (uint16_t)SID[base + 2U] |
                     ((uint16_t)(SID[base + 3U] & 15U) << 8);
    uint8_t ctrl = SID[base + 4U];
    uint8_t attack = SID[base + 5U] >> 4;
    uint8_t decay = SID[base + 5U] & 15U;
    uint8_t sustain = SID[base + 6U] >> 4;
    uint8_t waves = (uint8_t)(((ctrl >> 4) & 1U) + ((ctrl >> 5) & 1U) +
                              ((ctrl >> 6) & 1U) + ((ctrl >> 7) & 1U));

    if (ctrl & 0x80U)
        return (decay <= 4U && sustain <= 4U && freq > 5959U) ? 16U : 15U;
    if (freq < 1959U && sustain <= 2U && decay <= 5U)
        return 14U;
    if (freq < 3746U)
        return (ctrl & 0x10U) ? 3U : (((pulse >> 8) & 1U) ? 1U : 2U);
    if (waves >= 3U || (ctrl & 0x06U))
        return (freq < 8514U) ? 6U : 5U;
    if (attack >= 7U)
        return (ctrl & 0x20U) ? 13U : 12U;
    if ((ctrl & 0x40U) && sustain >= 10U && decay >= 6U)
        return 11U;
    if ((ctrl & 0x10U) && sustain <= 6U && decay <= 7U)
        return 4U;
    if ((ctrl & 0x40U) && pulse < 1150U)
        return 9U;
    if (ctrl & 0x40U)
        return 8U;
    return (ctrl & 0x20U) ? (((freq >> 9) & 1U) ? 7U : 10U) : 10U;
}

static void amiga_update_increment(uint8_t channel)
{
    amiga_voice_t* voice = &amiga_voice[channel];
    const amiga_bank_entry_t* e = voice->entry;
    uint64_t increment;
    uint64_t base;
    if (!e)
        return;
    base = ((uint64_t)e->native_rate * SID_MULTIPLIER * 65536U) / 1000000U;
    if (e->flags & AMIGA_SAMPLE_PERCUSSION)
        increment = base;
    else if (e->root_frequency_q16)
    {
        increment = ((uint64_t)e->native_rate * SID_MULTIPLIER *
                     amiga_frequency(channel) * 256U) / e->root_frequency_q16;
        if (increment < base / 8U) increment = base / 8U;
        if (increment > base * 8U) increment = base * 8U;
    }
    else
        increment = base;
    voice->increment_q16 = (uint32_t)increment;
}

static void amiga_start_voice(uint8_t channel)
{
    amiga_voice_t* voice = &amiga_voice[channel];
    voice->entry = amiga_sample_bank_entry(amiga_classify(channel));
    voice->pcm = amiga_sample_bank_pcm(voice->entry);
    voice->position_q16 = 0;
    voice->active = voice->entry != NULL;
    amiga_update_increment(channel);
}

void amiga_backend_reset(void)
{
    reset_SID();
    memset(amiga_voice, 0, sizeof(amiga_voice));
    amiga_filter_low = 0;
}

void amiga_backend_write(uint8_t address, uint8_t value)
{
    uint8_t channel;
    uint8_t reg;
    if (address < 21U && address % 7U == 4U && (value & 0x08U))
    {
        setreg(address, value & 0xFEU);
        SID[address] = value;
    }
    else
        setreg(address, value);
    if (address >= 21U)
        return;
    channel = address / 7U;
    reg = address - channel * 7U;
    if (reg <= 1U)
        amiga_update_increment(channel);
    else if (reg == 4U)
    {
        amiga_voice_t* voice = &amiga_voice[channel];
        uint8_t effective_gate = ((value & 0x09U) == 0x01U);
        voice->control = value;
        if (value & 0x08U)
        {
            voice->active = 0;
            voice->position_q16 = 0;
            voice->oscillator = 0;
        }
        if (effective_gate && !voice->gate)
            amiga_start_voice(channel);
        voice->gate = effective_gate;
    }
}

static int32_t amiga_sample(amiga_voice_t* voice)
{
    const amiga_bank_entry_t* e = voice->entry;
    uint32_t index = voice->position_q16 >> 16;
    uint32_t next;
    uint32_t fraction;
    int32_t a;
    int32_t b;
    int32_t value;
    uint32_t loop_end;
    uint32_t blend;
    if (!voice->active || !e || index >= e->length)
        return 0;
    next = index + 1U;
    if (next >= e->length)
        next = index;
    fraction = voice->position_q16 & 0xFFFFU;
    a = voice->pcm[index];
    b = voice->pcm[next];
    value = a + (((b - a) * (int32_t)fraction) >> 16);
    loop_end = e->loop_start + e->loop_length;
    if ((e->flags & AMIGA_SAMPLE_LOOP) && e->loop_length > 64U &&
        index + 32U >= loop_end && index < loop_end)
    {
        uint32_t loop_index = e->loop_start + index + 32U - loop_end;
        blend = index + 32U - loop_end;
        value = (value * (int32_t)(32U - blend) +
                 (int32_t)voice->pcm[loop_index] * (int32_t)blend) >> 5;
    }
    voice->position_q16 += voice->increment_q16;
    if ((e->flags & AMIGA_SAMPLE_LOOP) && e->loop_length &&
        (voice->position_q16 >> 16) >= loop_end)
    {
        uint32_t overshoot = (voice->position_q16 >> 16) - loop_end;
        while (overshoot >= e->loop_length)
            overshoot -= e->loop_length;
        voice->position_q16 = ((e->loop_start + overshoot) << 16) |
                              (voice->position_q16 & 0xFFFFU);
    }
    else if ((voice->position_q16 >> 16) >= e->length)
        voice->active = 0;
    return value;
}

void amiga_backend_cycle(void)
{
    static const uint8_t source[AMIGA_VOICES] = { 2, 0, 1 };
    struct EnvelopeGenerator* generators[AMIGA_VOICES] = { &gen1, &gen2, &gen3 };
    uint8_t wrapped[AMIGA_VOICES];
    uint8_t envelope[AMIGA_VOICES];
    uint8_t channel;
    int32_t filtered = 0;
    int32_t dry = 0;
    int32_t mix;
    int32_t scaled;

    for (channel = 0; channel < AMIGA_VOICES; ++channel)
    {
        uint32_t old = amiga_voice[channel].oscillator;
        amiga_voice[channel].oscillator =
            (old + (uint32_t)amiga_frequency(channel) * SID_MULTIPLIER) & 0xFFFFFFUL;
        wrapped[channel] = amiga_voice[channel].oscillator < old;
    }
    for (channel = 0; channel < AMIGA_VOICES; ++channel)
    {
        if ((amiga_voice[channel].control & 0x02U) && wrapped[source[channel]])
            amiga_voice[channel].position_q16 = 0;
        EnvelopeGenerator_clock_dt(generators[channel], SID_MULTIPLIER);
        envelope[channel] = EnvelopeGenerator_output(generators[channel]);
    }
    ADSR_volume_1 = envelope[0];
    ADSR_volume_2 = envelope[1];
    ADSR_volume_3 = envelope[2];

    for (channel = 0; channel < AMIGA_VOICES; ++channel)
    {
        amiga_voice_t* voice = &amiga_voice[channel];
        int32_t sample = amiga_sample(voice);
        if ((voice->control & 0x04U) && (amiga_voice[source[channel]].oscillator & 0x800000UL))
            sample = -sample;
        sample = (sample * (envelope[channel] >> 2)) >> 6;
        if (voice->entry)
            sample = (sample * voice->entry->volume) >> 6;
        if (channel == 2U && (SID[24] & 0x80U) && !(SID[23] & 4U))
            sample = 0;
        if (SID[23] & (1U << channel)) filtered += sample;
        else dry += sample;
    }

    {
        uint16_t cutoff = (uint16_t)((SID[21] & 7U) | ((uint16_t)SID[22] << 3));
        uint16_t coefficient = (uint16_t)(4U + (cutoff >> 3));
        int32_t high;
        int32_t band;
        int32_t filter_out = 0;
        if (coefficient > 240U) coefficient = 240U;
        band = filtered - amiga_filter_low;
        amiga_filter_low += (band * coefficient) >> 8;
        high = filtered - amiga_filter_low;
        if (SID[24] & 0x10U) filter_out += amiga_filter_low;
        if (SID[24] & 0x20U) filter_out += band >> 1;
        if (SID[24] & 0x40U) filter_out += high;
        mix = dry + ((SID[24] & 0x70U) ? filter_out : filtered);
    }

    /* Three signed 8-bit Paula channels and coarse 64-step volume provide the
     * deliberately crunchy Amiga character.  The old fixed gain was
     * mix*master*5/12.  The dedicated percent parameter now scales that gain;
     * its default of 20 is exactly five times quieter, while 100 restores the
     * original level.  All preceding stages remain int32_t and unclipped. */
    /* Keep this chain explicitly signed.  SID[] and parameter values are
     * unsigned; allowing one of them to control the usual arithmetic
     * conversions turns a negative PCM mix into a very large positive value,
     * which then clips every negative half-wave to DAC full scale. */
    scaled = mix * (int32_t)(SID[24] & 15U);
    scaled = (scaled *
              (int32_t)kungfusid_parameters.amiga_mix_volume_percent.value) /
             240;
    scaled = (scaled * (int32_t)sid_output_gain_percent) / 100;
    /* This is the only audio bound: clamp to the 12-bit DAC around midpoint.
     * Keep all 12 DAC bits.  The samples themselves retain their Paula-like
     * 8-bit character, while forcing the final value to 16-count steps would
     * turn low gain settings into a two-level, badly distorted waveform. */
    if (scaled < -2048) scaled = -2048;
    if (scaled > 2047) scaled = 2047;
    main_volume = (uint16_t)(scaled + 2048);
    main_volume_32bit = main_volume;
    SID[27] = (uint8_t)(amiga_voice[2].oscillator >> 16);
    SID[28] = envelope[2];
}
