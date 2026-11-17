#!/usr/bin/env python3
"""
Simple Serial Monitor for STM32F411 microWakeWord Assistant
Listens to USART2 (ST-Link Virtual COM Port or USB-UART adapter) at 115200 baud.
"""

import sys
import time
import glob

def find_serial_port():
    ports = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")
    return ports[0] if ports else None

def main():
    try:
        import serial
    except ImportError:
        print("[ERROR] pyserial is required. Install with: pip install pyserial")
        sys.exit(1)

    port = sys.argv[1] if len(sys.argv) > 1 else find_serial_port()
    if not port:
        print("[ERROR] No serial port specified and no /dev/ttyACM* or /dev/ttyUSB* device found.")
        print("Usage: python3 tools/serial_monitor.py [PORT]")
        sys.exit(1)

    baud = 115200
    print(f"[*] Opening serial monitor on {port} @ {baud} baud (Ctrl+C to quit)...")

    try:
        ser = serial.Serial(port, baud, timeout=1.0)
    except Exception as e:
        print(f"[ERROR] Failed to open {port}: {e}")
        sys.exit(1)

    try:
        while True:
            line = ser.readline().decode("utf-8", errors="replace")
            if line:
                # Highlight wake word detection lines
                if "WAKE WORD" in line or "Hey" in line:
                    print(f"\033[1;32m{line.rstrip()}\033[0m")
                elif "ERROR" in line:
                    print(f"\033[1;31m{line.rstrip()}\033[0m")
                else:
                    print(line.rstrip())
    except KeyboardInterrupt:
        print("\n[*] Exiting serial monitor.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
