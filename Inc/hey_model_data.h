#ifndef __HEY_MODEL_DATA_H
#define __HEY_MODEL_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char g_hey_model_data[];
extern const unsigned int g_hey_model_data_len;

#define HEY_MODEL_NAME                  "Hey Jarvis"
#define HEY_MODEL_PROBABILITY_CUTOFF    0.60f
#define HEY_MODEL_SLIDING_WINDOW_SIZE   5
#define HEY_MODEL_TENSOR_ARENA_SIZE     (36 * 1024)

#ifdef __cplusplus
}
#endif

#endif // __HEY_MODEL_DATA_H
