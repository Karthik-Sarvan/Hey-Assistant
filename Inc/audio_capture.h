#ifndef __AUDIO_CAPTURE_H
#define __AUDIO_CAPTURE_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SAMPLE_RATE_HZ     16000
#define AUDIO_PDM_BUFFER_SIZE    512   // 512 halfwords = 256 per half-buffer
#define AUDIO_PCM_HALF_SAMPLES   (AUDIO_PDM_BUFFER_SIZE / 2 / 4) // 64 PCM samples per DMA interrupt
#define AUDIO_RING_BUFFER_SIZE   2048  // Ring buffer size in 16-bit PCM samples (~128 ms)

typedef void (*AudioDataReadyCallback_t)(const int16_t *samples, size_t count);

/**
 * @brief Initialize I2S2, GPIO, DMA, and PDM filter for the MP45DT02 microphone
 * @param gain Decimation filter gain (default: 16)
 * @return HAL status (HAL_OK on success)
 */
HAL_StatusTypeDef AudioCapture_Init(int16_t gain);

/**
 * @brief Start continuous circular DMA audio recording
 */
HAL_StatusTypeDef AudioCapture_Start(void);

/**
 * @brief Stop audio recording
 */
HAL_StatusTypeDef AudioCapture_Stop(void);

/**
 * @brief Read available 16 kHz PCM audio samples from the ring buffer
 * @param buffer Output buffer
 * @param max_samples Maximum number of samples to read
 * @return Actual number of samples read
 */
size_t AudioCapture_Read(int16_t *buffer, size_t max_samples);

/**
 * @brief Number of samples currently available in the ring buffer
 */
size_t AudioCapture_Available(void);

/**
 * @brief Register callback called whenever a new chunk of PCM samples is ready
 */
void AudioCapture_SetCallback(AudioDataReadyCallback_t callback);

// DMA Interrupt handler (called by DMA1_Stream3_IRQHandler)
void AudioCapture_DMA_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // __AUDIO_CAPTURE_H
