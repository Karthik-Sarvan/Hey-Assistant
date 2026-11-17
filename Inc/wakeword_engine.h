#ifndef __WAKEWORD_ENGINE_H
#define __WAKEWORD_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "feature_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WakeWordDetectedCallback_t)(float confidence);

/**
 * @brief Initialize the microWakeWord TFLite Micro inference engine
 * @param probability_cutoff Cutoff threshold between 0.0f and 1.0f (e.g., 0.90f)
 * @return true on success, false on failure
 */
bool WakeWordEngine_Init(float probability_cutoff);

/**
 * @brief Feed a 40-feature slice from the audio preprocessor into the streaming model.
 *        Runs inference every 3 slices (30 ms).
 * @param features Pointer to 40 int8 features
 * @param out_confidence Output pointer for the current sliding-window confidence [0.0 - 1.0]
 * @return true if wake word "Hey Jarvis" was detected, false otherwise
 */
bool WakeWordEngine_ProcessSlice(const int8_t features[FEATURE_SLICE_SIZE], float *out_confidence);

/**
 * @brief Set the wake word detection callback
 */
void WakeWordEngine_SetCallback(WakeWordDetectedCallback_t callback);

/**
 * @brief Reset internal probability history and streaming state
 */
void WakeWordEngine_Reset(void);

/**
 * @brief Get the configured wake word name
 */
const char* WakeWordEngine_GetName(void);

#ifdef __cplusplus
}
#endif

#endif // __WAKEWORD_ENGINE_H
