#include "main.h"
#include "board.h"
#include "audio_capture.h"
#include "feature_provider.h"
#include "wakeword_engine.h"
#include <cstdio>

// Circular LED sequence: Green (PD12) -> Orange (PD13) -> Red (PD14) -> Blue (PD15)
static const BoardLed_t kChaserLeds[4] = {
    LED_GREEN,
    LED_ORANGE,
    LED_RED,
    LED_BLUE
};

constexpr uint32_t kChaserDurationMs = 1500;
constexpr uint32_t kChaserStepMs = 75;

static bool s_chaser_active = false;
static uint32_t s_chaser_end_tick = 0;
static uint32_t s_chaser_next_step_tick = 0;
static uint8_t s_chaser_step = 0;

static void StartLedChaser(void)
{
    s_chaser_active = true;
    s_chaser_step = 0;
    s_chaser_end_tick = Board_GetTicksMs() + kChaserDurationMs;
    s_chaser_next_step_tick = Board_GetTicksMs();
}

static void UpdateLedChaser(void)
{
    if (!s_chaser_active) {
        return;
    }

    uint32_t now = Board_GetTicksMs();
    if (now >= s_chaser_end_tick) {
        s_chaser_active = false;
        Board_LedOff(LED_GREEN);
        Board_LedOff(LED_ORANGE);
        Board_LedOff(LED_RED);
        Board_LedOff(LED_BLUE);
        return;
    }

    if (now >= s_chaser_next_step_tick) {
        s_chaser_next_step_tick = now + kChaserStepMs;

        Board_LedOff(LED_GREEN);
        Board_LedOff(LED_ORANGE);
        Board_LedOff(LED_RED);
        Board_LedOff(LED_BLUE);

        Board_LedOn(kChaserLeds[s_chaser_step]);
        s_chaser_step = (s_chaser_step + 1) % 4;
    }
}

// 10 ms audio chunk at 16 kHz
constexpr size_t kAudioChunkSize = 160;
static int16_t s_audio_chunk[kAudioChunkSize];

extern "C" void Application_OnWakeWord(float confidence)
{
    StartLedChaser();
    printf("Wake word detected (%d%% confidence)\r\n", static_cast<int>(confidence * 100.0f));
}

static void OnWakeWordCallback(float confidence)
{
    Application_OnWakeWord(confidence);
}

int main(void)
{
    Board_Init();

    // Ensure all LEDs start off
    Board_LedOff(LED_GREEN);
    Board_LedOff(LED_ORANGE);
    Board_LedOff(LED_RED);
    Board_LedOff(LED_BLUE);

    printf("STM32F411 microWakeWord (%s)\r\n", WakeWordEngine_GetName());

    if (!FeatureProvider_Init()) {
        printf("Error: failed to init feature provider\r\n");
        Board_LedOn(LED_RED);
        while (1) {}
    }

    if (!WakeWordEngine_Init(0.60f)) {
        printf("Error: failed to init wake word engine\r\n");
        Board_LedOn(LED_RED);
        while (1) {}
    }
    WakeWordEngine_SetCallback(OnWakeWordCallback);

    if (AudioCapture_Init(64) != HAL_OK || AudioCapture_Start() != HAL_OK) {
        printf("Error: failed to start audio capture\r\n");
        Board_LedOn(LED_RED);
        while (1) {}
    }

    printf("Listening...\r\n");

    int8_t features[FEATURE_SLICE_SIZE];

    while (1) {
        if (AudioCapture_Available() >= kAudioChunkSize) {
            size_t read_count = AudioCapture_Read(s_audio_chunk, kAudioChunkSize);

            size_t consumed = 0;
            bool feature_ready = FeatureProvider_Process(s_audio_chunk, read_count, features, &consumed);

            if (feature_ready) {
                float confidence = 0.0f;
                WakeWordEngine_ProcessSlice(features, &confidence);
            }
        }

        UpdateLedChaser();

        // Blue user button triggers the chaser for manual testing
        if (Board_ButtonRead()) {
            Application_OnWakeWord(1.0f);
            Board_DelayMs(300);
        }
    }

    return 0;
}
