#include "board.h"
#include <stdio.h>

static UART_HandleTypeDef huart2;

// SysTick interrupt handler: increments HAL tick counter every 1 ms
void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Fault handlers: Turn on Red LED (PD14) solid so crashes are immediately visible
void HardFault_Handler(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef gpio_led = {0};
    gpio_led.Pin = GPIO_PIN_14;
    gpio_led.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_led.Pull = GPIO_NOPULL;
    gpio_led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio_led);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    while (1) {
        // Halt
    }
}

void MemManage_Handler(void)
{
    HardFault_Handler();
}

void BusFault_Handler(void)
{
    HardFault_Handler();
}

void UsageFault_Handler(void)
{
    HardFault_Handler();
}

/**
 * @brief Configure system clock to 100 MHz
 * Robust multi-stage clock configuration for STM32F411-DISCO (MB1115D):
 * 1. Tries HSE Bypass (8 MHz MCO from ST-LINK MCU - default for MB1115D)
 * 2. Tries HSE Oscillator (if an external 8 MHz crystal is populated)
 * 3. Falls back to internal HSI (16 MHz) with PLL (100 MHz SYSCLK)
 * Guaranteed never to freeze or hang the MCU during boot.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Enable Power Control clock
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 1. Try HSE BYPASS (ST-LINK 8 MHz MCO square wave on OSC_IN)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 200;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 100 MHz
    RCC_OscInitStruct.PLL.PLLQ = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // 2. Try HSE ON (in case an external 8 MHz crystal is fitted)
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
            // 3. Fall back to internal HSI (16 MHz RC) -> 100 MHz SYSCLK
            RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
            RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
            RCC_OscInitStruct.HSIState = RCC_HSI_ON;
            RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
            RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
            RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
            RCC_OscInitStruct.PLL.PLLM = 16;  // 16 MHz / 16 = 1 MHz VCO in
            RCC_OscInitStruct.PLL.PLLN = 200; // 1 MHz * 200 = 200 MHz VCO
            RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 200 MHz / 2 = 100 MHz
            RCC_OscInitStruct.PLL.PLLQ = 4;
            if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
                // If PLL fails, safely continue on raw HSI (16 MHz)
                SystemCoreClockUpdate();
                return;
            }
        }
    }

    // Configure bus clock dividers (HCLK = 100 MHz, PCLK1 = 50 MHz, PCLK2 = 100 MHz)
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

    // Enable Flash Instruction & Data Cache (ART Accelerator) and Prefetch Buffer
    // Reduces Flash latency from 3 wait states to 0-wait-state equivalent performance!
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

    // Update CMSIS SystemCoreClock variable and reconfigure SysTick for 100 MHz
    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
}

void Board_Init(void)
{
    // Initialize HAL library and SysTick timer
    HAL_Init();

    // Enable GPIOD clock early so LEDs are ready immediately
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef gpio_led = {0};
    gpio_led.Pin = LED_GREEN_PIN | LED_ORANGE_PIN | LED_RED_PIN | LED_BLUE_PIN;
    gpio_led.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_led.Pull = GPIO_NOPULL;
    gpio_led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &gpio_led);

    // Turn off all LEDs initially
    HAL_GPIO_WritePin(LED_GPIO_PORT, gpio_led.Pin, GPIO_PIN_RESET);

    // Configure 100 MHz system clock
    SystemClock_Config();

    // Enable GPIOA clock for blue User Button (PA0)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio_btn = {0};
    gpio_btn.Pin = BUTTON_USER_PIN;
    gpio_btn.Mode = GPIO_MODE_INPUT;
    gpio_btn.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BUTTON_USER_PORT, &gpio_btn);

    // Optional UART at 115200 baud for debugging (if attached)
    Board_UART_Init(115200);
}

void Board_LedOn(BoardLed_t led)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, (uint16_t)led, GPIO_PIN_SET);
}

void Board_LedOff(BoardLed_t led)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, (uint16_t)led, GPIO_PIN_RESET);
}

void Board_LedToggle(BoardLed_t led)
{
    HAL_GPIO_TogglePin(LED_GPIO_PORT, (uint16_t)led);
}

bool Board_ButtonRead(void)
{
    return (HAL_GPIO_ReadPin(BUTTON_USER_PORT, BUTTON_USER_PIN) == GPIO_PIN_SET);
}

void Board_DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t Board_GetTicksMs(void)
{
    return HAL_GetTick();
}

void Board_UART_Init(uint32_t baudrate)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PA2 -> USART2_TX, PA3 -> USART2_RX
    GPIO_InitTypeDef gpio_uart = {0};
    gpio_uart.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio_uart.Mode = GPIO_MODE_AF_PP;
    gpio_uart.Pull = GPIO_PULLUP;
    gpio_uart.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_uart.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio_uart);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = baudrate;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void Board_UART_SendChar(char c)
{
    if (huart2.Instance != NULL) {
        HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10);
    }
}

void Board_UART_SendString(const char* str)
{
    if (!str) return;
    while (*str) {
        Board_UART_SendChar(*str++);
    }
}
