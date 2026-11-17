#include "wakeword_engine.h"
#include "hey_model_data.h"
#include "board.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_resource_variable.h"
#include "tensorflow/lite/micro/micro_allocator.h"

#include <cstring>
#include <algorithm>

namespace {

// Tensor arena in SRAM for TFLite Micro
constexpr size_t kTensorArenaSize = HEY_MODEL_TENSOR_ARENA_SIZE;
alignas(16) static uint8_t s_tensor_arena[kTensorArenaSize];

// TFLM components
static tflite::MicroMutableOpResolver<24> s_op_resolver;
static const tflite::Model* s_model = nullptr;
static tflite::MicroInterpreter* s_interpreter = nullptr;
static TfLiteTensor* s_input_tensor = nullptr;
static TfLiteTensor* s_output_tensor = nullptr;

// Sliding window and detection parameters
static uint8_t s_probability_cutoff_u8 = static_cast<uint8_t>(HEY_MODEL_PROBABILITY_CUTOFF * 255.0f);
constexpr size_t kSlidingWindowSize = HEY_MODEL_SLIDING_WINDOW_SIZE;
static uint8_t s_recent_probabilities[kSlidingWindowSize] = {0};
static size_t s_window_idx = 0;

// Diagnostic: peak probability seen since boot (accessible via SWD/debugger)
volatile uint8_t g_peak_probability = 0;

static uint8_t s_stride = 3;
static uint8_t s_current_stride_step = 0;

// Refractory cooldown period (ms) to avoid duplicate activations
constexpr uint32_t kCooldownMs = 1200;
static uint32_t s_last_detection_tick = 0;

static WakeWordDetectedCallback_t s_callback = nullptr;
static bool s_is_initialized = false;

} // namespace

bool WakeWordEngine_Init(float probability_cutoff)
{
    if (s_is_initialized) {
        return true;
    }

    if (probability_cutoff > 0.0f && probability_cutoff <= 1.0f) {
        s_probability_cutoff_u8 = static_cast<uint8_t>(probability_cutoff * 255.0f);
    }

    // Register all required streaming operations
    s_op_resolver.AddCallOnce();
    s_op_resolver.AddVarHandle();
    s_op_resolver.AddReshape();
    s_op_resolver.AddExpandDims();
    s_op_resolver.AddReadVariable();
    s_op_resolver.AddStridedSlice();
    s_op_resolver.AddConcatenation();
    s_op_resolver.AddAssignVariable();
    s_op_resolver.AddConv2D();
    s_op_resolver.AddMul();
    s_op_resolver.AddAdd();
    s_op_resolver.AddMean();
    s_op_resolver.AddFullyConnected();
    s_op_resolver.AddLogistic();
    s_op_resolver.AddDepthwiseConv2D();
    s_op_resolver.AddQuantize();
    s_op_resolver.AddDequantize();

    // Map model from flash memory
    s_model = tflite::GetModel(g_hey_model_data);
    if (s_model->version() != TFLITE_SCHEMA_VERSION) {
        return false;
    }

    // Create allocator and resource variables required for stateful streaming ops (VarHandle)
    static tflite::MicroAllocator* s_allocator = tflite::MicroAllocator::Create(
        s_tensor_arena, kTensorArenaSize);
    if (!s_allocator) {
        return false;
    }

    static tflite::MicroResourceVariables* s_resource_variables =
        tflite::MicroResourceVariables::Create(s_allocator, 24);
    if (!s_resource_variables) {
        return false;
    }

    // Allocate interpreter with resource variables attached
    static tflite::MicroInterpreter static_interpreter(
        s_model, s_op_resolver, s_allocator, s_resource_variables);
    s_interpreter = &static_interpreter;

    if (s_interpreter->AllocateTensors() != kTfLiteOk) {
        return false;
    }

    s_input_tensor = s_interpreter->input(0);
    s_output_tensor = s_interpreter->output(0);

    // Verify input tensor format [1, stride, 40]
    if (s_input_tensor->dims->size >= 2) {
        s_stride = s_input_tensor->dims->data[1];
    } else {
        s_stride = 3;
    }

    s_current_stride_step = 0;
    WakeWordEngine_Reset();

    s_is_initialized = true;
    return true;
}

void WakeWordEngine_Reset(void)
{
    s_current_stride_step = 0;
    s_window_idx = 0;
    std::memset(s_recent_probabilities, 0, sizeof(s_recent_probabilities));
}

bool WakeWordEngine_ProcessSlice(const int8_t features[FEATURE_SLICE_SIZE], float *out_confidence)
{
    if (!s_is_initialized || !s_input_tensor || !s_output_tensor) {
        if (out_confidence) *out_confidence = 0.0f;
        return false;
    }

    // Copy the newest 40-feature slice into the model's strided input buffer
    int8_t* input_data = tflite::GetTensorData<int8_t>(s_input_tensor);
    std::memcpy(input_data + (s_current_stride_step * FEATURE_SLICE_SIZE),
                features, FEATURE_SLICE_SIZE);
    s_current_stride_step++;

    // Only invoke inference every stride steps (typically 3 slices = 30 ms)
    if (s_current_stride_step < s_stride) {
        if (out_confidence) {
            uint32_t sum = 0;
            for (size_t i = 0; i < kSlidingWindowSize; i++) {
                sum += s_recent_probabilities[i];
            }
            *out_confidence = (static_cast<float>(sum) / (kSlidingWindowSize * 255.0f));
        }
        return false;
    }

    // Reset stride counter
    s_current_stride_step = 0;

    // Run streaming inference
    if (s_interpreter->Invoke() != kTfLiteOk) {
        return false;
    }

    // Output is a single uint8 probability [0 - 255]
    uint8_t current_prob = s_output_tensor->data.uint8[0];

    if (current_prob > g_peak_probability) {
        g_peak_probability = current_prob;
    }

    // Store in sliding window
    s_recent_probabilities[s_window_idx] = current_prob;
    s_window_idx = (s_window_idx + 1) % kSlidingWindowSize;

    // Calculate rolling average probability
    uint32_t sum = 0;
    for (size_t i = 0; i < kSlidingWindowSize; i++) {
        sum += s_recent_probabilities[i];
    }
    uint8_t avg_prob = static_cast<uint8_t>(sum / kSlidingWindowSize);
    float confidence = static_cast<float>(avg_prob) / 255.0f;

    if (out_confidence) {
        *out_confidence = confidence;
    }

    // Check if detected: either rolling average over the sliding window reaches the cutoff,
    // or a strong instant peak slice occurs.
    uint32_t current_tick = Board_GetTicksMs();
    if (avg_prob >= s_probability_cutoff_u8 || current_prob >= (s_probability_cutoff_u8 + 20)) {
        if ((current_tick - s_last_detection_tick) > kCooldownMs) {
            s_last_detection_tick = current_tick;
            WakeWordEngine_Reset();

            if (s_callback) {
                float report_conf = (static_cast<float>(current_prob) / 255.0f);
                if (confidence > report_conf) report_conf = confidence;
                s_callback(report_conf);
            }
            return true;
        }
    }

    return false;
}

void WakeWordEngine_SetCallback(WakeWordDetectedCallback_t callback)
{
    s_callback = callback;
}

const char* WakeWordEngine_GetName(void)
{
    return HEY_MODEL_NAME;
}
