# STM32F411E-DISCO microWakeWord
TARGET = hey_assistant
BUILD_DIR = build

# Toolchain configuration: check if arm-none-eabi-g++ is on PATH, else fallback to PlatformIO toolchain
ifneq (, $(shell which arm-none-eabi-g++ 2>/dev/null))
  PREFIX = arm-none-eabi-
else ifneq (, $(wildcard $(HOME)/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-g++))
  PREFIX = $(HOME)/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-
else
  PREFIX = arm-none-eabi-
endif

CC  = $(PREFIX)gcc
CXX = $(PREFIX)g++
CP  = $(PREFIX)objcopy
SZ  = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# CPU and FPU flags
CPU   = -mcpu=cortex-m4
FPU   = -mfpu=fpv4-sp-d16
FLOAT = -mfloat-abi=hard
MCU   = $(CPU) -mthumb $(FPU) $(FLOAT)

# Macros
DEFS = -DSTM32F411xE -DUSE_HAL_DRIVER

# Include paths
INCLUDES = \
  -IInc \
  -IDrivers/CMSIS/Core/Include \
  -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IDrivers/STM32F4xx_HAL_Driver/Inc \
  -IMiddlewares \
  -IMiddlewares/tensorflow \
  -IMiddlewares/third_party/flatbuffers/include \
  -IMiddlewares/third_party/gemmlowp \
  -IMiddlewares/third_party/ruy \
  -IMiddlewares/third_party/kissfft \
  -IMiddlewares/microfrontend/lib

# C Source files
C_SOURCES = \
  Src/board.c \
  Src/audio_capture.c \
  Src/pdm_filter.c \
  Src/hey_model_data.c \
  Src/syscalls.c \
  Src/system_stm32f4xx.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c

# C++ Source files (.cpp)
CPP_SOURCES = \
  Src/main.cpp \
  Src/feature_provider.cpp \
  Src/wakeword_engine.cpp

# C++ Source files (.cc)
CC_SOURCES = \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/frontend.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/frontend_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/filterbank.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/filterbank_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/fft.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/fft_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/kiss_fft_int16.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/log_lut.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/log_scale.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/log_scale_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/noise_reduction.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/noise_reduction_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/pcan_gain_control.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/pcan_gain_control_util.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/window.cc \
  Middlewares/tensorflow/lite/experimental/microfrontend/lib/window_util.cc \
  Middlewares/tensorflow/lite/micro/micro_interpreter.cc \
  Middlewares/tensorflow/lite/micro/micro_allocator.cc \
  Middlewares/tensorflow/lite/micro/micro_allocation_info.cc \
  Middlewares/tensorflow/lite/micro/memory_helpers.cc \
  Middlewares/tensorflow/lite/micro/micro_resource_variable.cc \
  Middlewares/tensorflow/lite/micro/micro_context.cc \
  Middlewares/tensorflow/lite/micro/micro_interpreter_context.cc \
  Middlewares/tensorflow/lite/micro/micro_interpreter_graph.cc \
  Middlewares/tensorflow/lite/micro/micro_profiler.cc \
  Middlewares/tensorflow/lite/micro/micro_utils.cc \
  Middlewares/tensorflow/lite/micro/micro_log.cc \
  Middlewares/tensorflow/lite/micro/system_setup.cc \
  Middlewares/tensorflow/lite/micro/flatbuffer_conversions.cc \
  Middlewares/tensorflow/lite/micro/flatbuffer_utils.cc \
  Middlewares/tensorflow/lite/micro/debug_log.cc \
  Middlewares/tensorflow/lite/micro/micro_common.cc \
  Middlewares/tensorflow/lite/micro/micro_op_resolver.cc \
  Middlewares/tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.cc \
  Middlewares/tensorflow/lite/micro/memory_planner/greedy_memory_planner.cc \
  Middlewares/tensorflow/lite/micro/memory_planner/linear_memory_planner.cc \
  Middlewares/tensorflow/lite/micro/kernels/kernel_util.cc \
  Middlewares/tensorflow/lite/micro/kernels/call_once.cc \
  Middlewares/tensorflow/lite/micro/kernels/var_handle.cc \
  Middlewares/tensorflow/lite/micro/kernels/read_variable.cc \
  Middlewares/tensorflow/lite/micro/kernels/assign_variable.cc \
  Middlewares/tensorflow/lite/micro/kernels/expand_dims.cc \
  Middlewares/tensorflow/lite/micro/kernels/reshape.cc \
  Middlewares/tensorflow/lite/micro/kernels/reshape_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/strided_slice.cc \
  Middlewares/tensorflow/lite/micro/kernels/strided_slice_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/concatenation.cc \
  Middlewares/tensorflow/lite/micro/kernels/conv.cc \
  Middlewares/tensorflow/lite/micro/kernels/conv_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/depthwise_conv.cc \
  Middlewares/tensorflow/lite/micro/kernels/depthwise_conv_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/mul.cc \
  Middlewares/tensorflow/lite/micro/kernels/mul_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/add.cc \
  Middlewares/tensorflow/lite/micro/kernels/add_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/reduce.cc \
  Middlewares/tensorflow/lite/micro/kernels/reduce_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/fully_connected.cc \
  Middlewares/tensorflow/lite/micro/kernels/fully_connected_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/activations.cc \
  Middlewares/tensorflow/lite/micro/kernels/activations_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/logistic.cc \
  Middlewares/tensorflow/lite/micro/kernels/logistic_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/quantize.cc \
  Middlewares/tensorflow/lite/micro/kernels/quantize_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/dequantize.cc \
  Middlewares/tensorflow/lite/micro/kernels/dequantize_common.cc \
  Middlewares/tensorflow/lite/micro/kernels/internal/quantization_util.cc \
  Middlewares/tensorflow/lite/micro/kernels/internal/portable_tensor_utils.cc \
  Middlewares/tensorflow/lite/micro/kernels/internal/common.cc \
  Middlewares/tensorflow/lite/micro/kernels/internal/tensor_ctypes.cc

# Assembly sources
ASM_SOURCES = startup_stm32f411xe.s

# Compiler and Linker flags
COMMON_FLAGS = $(MCU) $(DEFS) $(INCLUDES) -O3 -g -Wall -fdata-sections -ffunction-sections
CFLAGS   = $(COMMON_FLAGS) -std=c11
CXXFLAGS = $(COMMON_FLAGS) -std=c++17 -fno-rtti -fno-exceptions -fno-threadsafe-statics

# Linker script & flags
LDSCRIPT = STM32F411VETX_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
           --specs=nano.specs --specs=nosys.specs -lc -lm -lnosys

# Object files
OBJS_C   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJS_CPP = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
OBJS_CC  = $(patsubst %.cc,$(BUILD_DIR)/%.o,$(CC_SOURCES))
OBJS_ASM = $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS  = $(OBJS_C) $(OBJS_CPP) $(OBJS_CC) $(OBJS_ASM)

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@echo "Build complete: $(BUILD_DIR)/$(TARGET).bin"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo "CXX $<"
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "AS  $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "LD  $@"
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@$(BIN) $< $@

# Flash target: tries st-flash first, then openocd as fallback
flash: all
	@echo "Flashing $(BUILD_DIR)/$(TARGET).bin to STM32F411-DISCO..."
	@st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x08000000 || \
	 openocd -f board/stm32f4discovery.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Clean target
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash
