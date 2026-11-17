#ifndef __FEATURE_PROVIDER_H
#define __FEATURE_PROVIDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FEATURE_SLICE_SIZE      40   // 40 Mel filterbank channels
#define AUDIO_WINDOW_SAMPLES    480  // 30 ms @ 16 kHz
#define AUDIO_STEP_SAMPLES      160  // 10 ms @ 16 kHz

/**
 * @brief Initialize the microfrontend audio preprocessor
 * @return true on success, false on failure
 */
bool FeatureProvider_Init(void);

/**
 * @brief Free feature provider state resources
 */
void FeatureProvider_Deinit(void);

/**
 * @brief Feed raw 16 kHz PCM audio samples into the preprocessor.
 *        If enough samples have accumulated (10 ms step), it produces a 40-feature slice.
 * @param samples Input 16-bit PCM audio samples
 * @param sample_count Number of samples provided
 * @param out_features Output buffer for 40 int8 features (must be at least 40 bytes)
 * @param samples_consumed Returns how many input samples were consumed
 * @return true if a 40-feature slice was generated, false otherwise
 */
bool FeatureProvider_Process(const int16_t *samples, size_t sample_count,
                             int8_t out_features[FEATURE_SLICE_SIZE], size_t *samples_consumed);

#ifdef __cplusplus
}
#endif

#endif // __FEATURE_PROVIDER_H
