# Hey Assistant

On-device wake word detection for the STM32F411E-DISCO board using microWakeWord and TensorFlow Lite for Microcontrollers. Listens continuously using the onboard MP45DT02 MEMS microphone and detects "Hey Jarvis" fully offline.

## Hardware

- Board: STM32F411E-DISCO (Cortex-M4 @ 100 MHz)
- Microphone: Built-in MP45DT02 MEMS mic (PDM on I2S2)
- LEDs: All LEDs remain off while listening. When "Hey Jarvis" is detected, an LED chaser animation runs across Green -> Orange -> Red -> Blue for 1.5 seconds.
- User button (B1, PA0): Triggers the LED chaser for manual testing.
- Serial debug (USART2): PA2 (TX), PA3 (RX) at 115200 baud.

## Building and Flashing

Requirements: `arm-none-eabi-gcc`, `make`, and `stlink` (or `openocd`).

```bash
make -j
make flash
```

## Changing the Model

To swap in a different microWakeWord `.tflite` model:

```bash
python3 tools/train_or_convert.py path/to/model.tflite "Wake Word Name"
make clean && make -j
make flash
```

## Repository Structure

- `Src/` - Application entry, board init, audio capture, PDM filter, TFLM engine
- `Inc/` - Header files and configuration
- `Middlewares/` - TFLite Micro, CMSIS, audio preprocessor
- `tools/` - Model conversion and serial monitor scripts
