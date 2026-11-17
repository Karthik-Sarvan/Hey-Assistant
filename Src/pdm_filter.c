#include "pdm_filter.h"

void PDMFilter_Init(PDMFilter_t *filter, int16_t gain)
{
    if (!filter) return;
    filter->i0 = filter->i1 = filter->i2 = 0;
    filter->c0_prev = filter->c1_prev = filter->c2_prev = 0;
    filter->hp_x = 0.0f;
    filter->hp_y = 0.0f;
    filter->gain = (gain > 0) ? gain : 32;
}

// 3rd-order CIC decimation filter with 64x decimation:
// 1.024 MHz PDM bitstream (64 bits = 4 x 16-bit words) -> 16 kHz 16-bit PCM sample
void PDMFilter_Process(PDMFilter_t *filter, const uint16_t *pdm_in, int16_t *pcm_out, size_t pcm_samples)
{
    if (!filter || !pdm_in || !pcm_out) return;

    int32_t i0 = filter->i0;
    int32_t i1 = filter->i1;
    int32_t i2 = filter->i2;

    int32_t c0_prev = filter->c0_prev;
    int32_t c1_prev = filter->c1_prev;
    int32_t c2_prev = filter->c2_prev;

    float hp_x = filter->hp_x;
    float hp_y = filter->hp_y;
    int32_t gain = filter->gain;

    size_t pdm_idx = 0;

    for (size_t s = 0; s < pcm_samples; s++) {
        // Process 64 bits = 4 words of 16-bit PDM data
        // MSB-first bit order (matches STM32 hardware I2S MSB-first shift order)
        for (int w = 0; w < 4; w++) {
            uint16_t word = pdm_in[pdm_idx++];
            for (int b = 15; b >= 0; b--) {
                int32_t bit_val = ((word >> b) & 1) ? 1 : -1;
                i0 += bit_val;
                i1 += i0;
                i2 += i1;
            }
        }

        // Comb section (decimated by 64)
        int32_t d0 = i2 - c0_prev;
        c0_prev = i2;

        int32_t d1 = d0 - c1_prev;
        c1_prev = d0;

        int32_t d2 = d1 - c2_prev;
        c2_prev = d1;

        // Gain scaling:
        // CIC gain is R^3 = 64^3 = 262144.
        // Scale by (d2 * gain) >> 8. For gain=32, peak full scale maps to 32768.
        int32_t sample = (d2 * gain) >> 8;

        // DC Blocker (IIR High-Pass filter): y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
        // Single-precision hardware FPU ensures smooth zero-decay without integer lockup
        float x_in = (float)sample;
        float hp_out = x_in - hp_x + (0.995f * hp_y);
        hp_x = x_in;
        hp_y = hp_out;

        // Clamp to 16-bit signed integer [-32768, 32767]
        int32_t out_val = (int32_t)hp_out;
        if (out_val > 32767) out_val = 32767;
        else if (out_val < -32768) out_val = -32768;

        pcm_out[s] = (int16_t)out_val;
    }

    filter->i0 = i0;
    filter->i1 = i1;
    filter->i2 = i2;

    filter->c0_prev = c0_prev;
    filter->c1_prev = c1_prev;
    filter->c2_prev = c2_prev;

    filter->hp_x = hp_x;
    filter->hp_y = hp_y;
}
