# Black Box Forensic Crash Reporter

A crash detection and forensic logging system for ATmega328P (Arduino Uno).

## Features

- **Watchdog Timer monitoring** - Detects system hangs and saves state before reset
- **EEPROM crash storage** - Crash data survives power loss and resets
- **Post-mortem analysis** - Displays crash reason, stack pointer, and reset flags on boot
- **Interactive test menu** - 5 options to trigger and test crash scenarios

## Usage

1. Build: `make`
2. Flash: `make flash`
3. Monitor: `screen /dev/cu.usbmodem101 9600`

## Menu Options

1. Trigger watchdog timeout (infinite loop)
2. Trigger explicit crash dump
3. Clear EEPROM crash data
4. Read current stack pointer
5. Test deep recursion (stack stress)