#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Application hook: called whenever "Hey Jarvis" is detected
void Application_OnWakeWord(float confidence);

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H
