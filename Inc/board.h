#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED definitions for STM32F411E-DISCO
#define LED_GREEN_PIN    GPIO_PIN_12
#define LED_ORANGE_PIN   GPIO_PIN_13
#define LED_RED_PIN      GPIO_PIN_14
#define LED_BLUE_PIN     GPIO_PIN_15
#define LED_GPIO_PORT    GPIOD

typedef enum {
    LED_GREEN  = LED_GREEN_PIN,
    LED_ORANGE = LED_ORANGE_PIN,
    LED_RED    = LED_RED_PIN,
    LED_BLUE   = LED_BLUE_PIN
} BoardLed_t;

// Push button
#define BUTTON_USER_PIN       GPIO_PIN_0
#define BUTTON_USER_PORT      GPIOA

// Board functions
void Board_Init(void);
void Board_LedOn(BoardLed_t led);
void Board_LedOff(BoardLed_t led);
void Board_LedToggle(BoardLed_t led);
bool Board_ButtonRead(void);
void Board_DelayMs(uint32_t ms);
uint32_t Board_GetTicksMs(void);

// UART logging
void Board_UART_Init(uint32_t baudrate);
void Board_UART_SendChar(char c);
void Board_UART_SendString(const char* str);

#ifdef __cplusplus
}
#endif

#endif // __BOARD_H
