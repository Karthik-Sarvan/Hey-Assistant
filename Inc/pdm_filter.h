#ifndef __PDM_FILTER_H
#define __PDM_FILTER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PDM_DECIMATION_FACTOR   64

typedef struct {
    // CIC Integrator states (3 stages)
    int32_t i0, i1, i2;
    // CIC Comb delay states (3 stages)
    int32_t c0, c1, c2;
    int32_t c0_prev, c1_prev, c2_prev;
    // High-pass filter state (DC blocker) using float to prevent integer truncation lockup
    float hp_x;
    float hp_y;
    // Gain scale
    int16_t gain;
} PDMFilter_t;

/**
 * @brief Initialize PDM filter instance
 * @param filter Pointer to filter struct
 * @param gain Gain multiplier (default: 16)
 */
void PDMFilter_Init(PDMFilter_t *filter, int16_t gain);

/**
 * @brief Decimate raw PDM buffer into 16-bit signed PCM audio samples.
 *        Decimation ratio = 64 (64 PDM bits -> 1 PCM sample).
 * @param filter Pointer to filter struct
 * @param pdm_in Pointer to input PDM words (16-bit words)
 * @param pcm_out Pointer to output PCM buffer (16-bit signed samples)
 * @param pcm_samples Number of PCM samples to produce
 */
void PDMFilter_Process(PDMFilter_t *filter, const uint16_t *pdm_in, int16_t *pcm_out, size_t pcm_samples);

#ifdef __cplusplus
}
#endif

#endif // __PDM_FILTER_H
