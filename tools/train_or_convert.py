#!/usr/bin/env python3
"""
Model Converter for microWakeWord on STM32F411
Converts a microWakeWord .tflite model into C array source files:
  - Inc/hey_model_data.h
  - Src/hey_model_data.c
"""

import sys
import os

def convert_tflite_to_c(tflite_path, output_c, output_h, wake_word_name="Hey Jarvis"):
    if not os.path.exists(tflite_path):
        print(f"Error: file not found: {tflite_path}")
        sys.exit(1)

    with open(tflite_path, "rb") as f:
        data = f.read()

    size_bytes = len(data)
    print(f"Reading {tflite_path} ({size_bytes} bytes)...")

    with open(output_h, "w") as f:
        f.write("#ifndef __HEY_MODEL_DATA_H\n")
        f.write("#define __HEY_MODEL_DATA_H\n\n")
        f.write("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
        f.write("extern const unsigned char g_hey_model_data[];\n")
        f.write("extern const unsigned int g_hey_model_data_len;\n\n")
        f.write(f'#define HEY_MODEL_NAME                  "{wake_word_name}"\n')
        f.write("#define HEY_MODEL_PROBABILITY_CUTOFF    0.60f\n")
        f.write("#define HEY_MODEL_SLIDING_WINDOW_SIZE   5\n")
        f.write("#define HEY_MODEL_TENSOR_ARENA_SIZE     (36 * 1024)\n\n")
        f.write("#ifdef __cplusplus\n}\n#endif\n\n")
        f.write("#endif // __HEY_MODEL_DATA_H\n")

    with open(output_c, "w") as f:
        f.write('#include "hey_model_data.h"\n\n')
        f.write("const unsigned char g_hey_model_data[] = {\n")
        for i, byte in enumerate(data):
            f.write(f"0x{byte:02x}, ")
            if (i + 1) % 12 == 0:
                f.write("\n  ")
        f.write("\n};\n")
        f.write(f"const unsigned int g_hey_model_data_len = {size_bytes};\n")

    print(f"Generated {output_h} and {output_c}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 tools/train_or_convert.py <path_to_model.tflite> [wake_word_name]")
        sys.exit(1)

    tflite_path = sys.argv[1]
    name = sys.argv[2] if len(sys.argv) > 2 else "Hey Jarvis"

    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_c = os.path.join(base_dir, "Src", "hey_model_data.c")
    output_h = os.path.join(base_dir, "Inc", "hey_model_data.h")

    convert_tflite_to_c(tflite_path, output_c, output_h, name)

if __name__ == "__main__":
    main()
