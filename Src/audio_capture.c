#include "audio_capture.h"
#include "pdm_filter.h"
#include <string.h>

static I2S_HandleTypeDef hi2s2;
static DMA_HandleTypeDef hdma_i2s2_rx;
static PDMFilter_t pdm_filter;

// Double buffer for raw PDM bits from I2S DMA
static uint16_t pdm_dma_buffer[AUDIO_PDM_BUFFER_SIZE];

// Circular ring buffer for converted 16 kHz PCM audio samples
static int16_t pcm_ring_buffer[AUDIO_RING_BUFFER_SIZE];
static volatile size_t ring_head = 0;
static volatile size_t ring_tail = 0;

static AudioDataReadyCallback_t data_callback = NULL;

static void AudioCapture_I2SClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    
    // Choose division factor based on whether main PLL source is HSE (8MHz) or HSI (16MHz)
    uint32_t pll_src = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
    PeriphClkInitStruct.PLLI2S.PLLI2SM = (pll_src == RCC_PLLCFGR_PLLSRC_HSE) ? 8 : 16;
    PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
    PeriphClkInitStruct.PLLI2S.PLLI2SR = 6; // 192 MHz / 6 = 32 MHz I2S clock
    
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

HAL_StatusTypeDef AudioCapture_Init(int16_t gain)
{
    // Initialize PDM decimation filter (default gain: 28)
    PDMFilter_Init(&pdm_filter, (gain > 0) ? gain : 28);

    // Reset ring buffer
    ring_head = 0;
    ring_tail = 0;

    // Configure I2S clock via PLLI2S
    AudioCapture_I2SClock_Config();

    // Enable Peripheral Clocks
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // Configure GPIO Pins: PB10 -> I2S2_CK, PC3 -> I2S2_SD (from MP45DT02 DOUT)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure I2S2 for 1.024 MHz PDM clock (AudioFreq = 32000, 32-bit frame = 1.024 MHz)
    // Standard configuration for MP45DT02 MEMS microphone on STM32F4 Discovery
    hi2s2.Instance = SPI2;
    hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
    hi2s2.Init.Standard = I2S_STANDARD_LSB;
    hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    hi2s2.Init.AudioFreq = 32000;
    hi2s2.Init.CPOL = I2S_CPOL_HIGH;
    hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    if (HAL_I2S_Init(&hi2s2) != HAL_OK) {
        return HAL_ERROR;
    }

    // Configure DMA1 Stream 3 Channel 0 (SPI2_RX / I2S2_EXT_RX)
    hdma_i2s2_rx.Instance = DMA1_Stream3;
    hdma_i2s2_rx.Init.Channel = DMA_CHANNEL_0;
    hdma_i2s2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2s2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2s2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2s2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_i2s2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_i2s2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_i2s2_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_i2s2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_i2s2_rx) != HAL_OK) {
        return HAL_ERROR;
    }

    __HAL_LINKDMA(&hi2s2, hdmarx, hdma_i2s2_rx);

    // Configure DMA Interrupt
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

    return HAL_OK;
}

HAL_StatusTypeDef AudioCapture_Start(void)
{
    return HAL_I2S_Receive_DMA(&hi2s2, pdm_dma_buffer, AUDIO_PDM_BUFFER_SIZE);
}

HAL_StatusTypeDef AudioCapture_Stop(void)
{
    return HAL_I2S_DMAStop(&hi2s2);
}

static void ProcessPDMHalf(const uint16_t *pdm_half)
{
    int16_t pcm_samples[AUDIO_PCM_HALF_SAMPLES];
    PDMFilter_Process(&pdm_filter, pdm_half, pcm_samples, AUDIO_PCM_HALF_SAMPLES);

    // Push into ring buffer
    for (size_t i = 0; i < AUDIO_PCM_HALF_SAMPLES; i++) {
        size_t next_head = (ring_head + 1) % AUDIO_RING_BUFFER_SIZE;
        if (next_head != ring_tail) {
            pcm_ring_buffer[ring_head] = pcm_samples[i];
            ring_head = next_head;
        } else {
            // Overrun: advance tail to discard oldest sample
            pcm_ring_buffer[ring_head] = pcm_samples[i];
            ring_head = next_head;
            ring_tail = (ring_tail + 1) % AUDIO_RING_BUFFER_SIZE;
        }
    }

    if (data_callback) {
        data_callback(pcm_samples, AUDIO_PCM_HALF_SAMPLES);
    }
}

// Half-transfer callback: process first half of PDM buffer
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        ProcessPDMHalf(&pdm_dma_buffer[0]);
    }
}

// Transfer-complete callback: process second half of PDM buffer
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        ProcessPDMHalf(&pdm_dma_buffer[AUDIO_PDM_BUFFER_SIZE / 2]);
    }
}

void AudioCapture_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2s2_rx);
}

void DMA1_Stream3_IRQHandler(void)
{
    AudioCapture_DMA_IRQHandler();
}

size_t AudioCapture_Available(void)
{
    size_t head = ring_head;
    size_t tail = ring_tail;
    if (head >= tail) {
        return head - tail;
    }
    return AUDIO_RING_BUFFER_SIZE - tail + head;
}

size_t AudioCapture_Read(int16_t *buffer, size_t max_samples)
{
    if (!buffer || max_samples == 0) return 0;

    size_t count = 0;
    while (count < max_samples && ring_tail != ring_head) {
        buffer[count++] = pcm_ring_buffer[ring_tail];
        ring_tail = (ring_tail + 1) % AUDIO_RING_BUFFER_SIZE;
    }
    return count;
}

void AudioCapture_SetCallback(AudioDataReadyCallback_t callback)
{
    data_callback = callback;
}
