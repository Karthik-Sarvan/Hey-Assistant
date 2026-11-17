#include "feature_provider.h"
#include "tensorflow/lite/experimental/microfrontend/lib/frontend.h"
#include "tensorflow/lite/experimental/microfrontend/lib/frontend_util.h"
#include <algorithm>

static struct FrontendConfig s_frontend_config;
static struct FrontendState s_frontend_state;
static bool s_is_initialized = false;

bool FeatureProvider_Init(void)
{
    if (s_is_initialized) {
        return true;
    }

    // Configure preprocessor settings matching microWakeWord training specifications
    s_frontend_config.window.size_ms = 30;
    s_frontend_config.window.step_size_ms = 10;

    s_frontend_config.filterbank.num_channels = FEATURE_SLICE_SIZE;
    s_frontend_config.filterbank.lower_band_limit = 125.0f;
    s_frontend_config.filterbank.upper_band_limit = 7500.0f;

    s_frontend_config.noise_reduction.smoothing_bits = 10;
    s_frontend_config.noise_reduction.even_smoothing = 0.025f;
    s_frontend_config.noise_reduction.odd_smoothing = 0.06f;
    s_frontend_config.noise_reduction.min_signal_remaining = 0.05f;

    s_frontend_config.pcan_gain_control.enable_pcan = 1;
    s_frontend_config.pcan_gain_control.strength = 0.95f;
    s_frontend_config.pcan_gain_control.offset = 80.0f;
    s_frontend_config.pcan_gain_control.gain_bits = 21;

    s_frontend_config.log_scale.enable_log = 1;
    s_frontend_config.log_scale.scale_shift = 6;

    if (!FrontendPopulateState(&s_frontend_config, &s_frontend_state, 16000)) {
        return false;
    }

    s_is_initialized = true;
    return true;
}

void FeatureProvider_Deinit(void)
{
    if (s_is_initialized) {
        FrontendFreeStateContents(&s_frontend_state);
        s_is_initialized = false;
    }
}

bool FeatureProvider_Process(const int16_t *samples, size_t sample_count,
                             int8_t out_features[FEATURE_SLICE_SIZE], size_t *samples_consumed)
{
    if (!s_is_initialized || !samples || sample_count == 0) {
        if (samples_consumed) *samples_consumed = 0;
        return false;
    }

    size_t processed = 0;
    struct FrontendOutput output = FrontendProcessSamples(&s_frontend_state, samples, sample_count, &processed);
    if (samples_consumed) {
        *samples_consumed = processed;
    }

    if (output.size == 0) {
        return false;
    }

    // Scale and quantize 16-bit filterbank values to int8 [-128, 127]
    // Matches the microWakeWord int8 quantization formula:
    // val = (feature * 256) / (25.6 * 26.0) - 128
    constexpr int32_t value_scale = 256;
    constexpr int32_t value_div = 666; // 25.6 * 26.0 = 665.6 (approx 666)

    size_t copy_size = std::min(output.size, (size_t)FEATURE_SLICE_SIZE);
    for (size_t i = 0; i < copy_size; ++i) {
        int32_t val = ((output.values[i] * value_scale) + (value_div / 2)) / value_div;
        val -= 128; // Subtract 128 to convert to signed int8
        if (val > 127) val = 127;
        if (val < -128) val = -128;
        out_features[i] = static_cast<int8_t>(val);
    }

    return true;
}
