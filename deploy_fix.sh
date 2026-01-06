#!/bin/bash
# Script to rebuild and upload firmware (run from a NEW terminal)

cd "/Users/camal/Desktop/crash detector"

echo "Killing all screen sessions..."
killall -9 screen 2>/dev/null
sleep 2

echo "Building firmware..."
avr-gcc -DF_CPU=16000000UL -Os -std=gnu99 -mmcu=atmega328p -c src/main.c -o build/main.o
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

avr-gcc -mmcu=atmega328p -o build/main.elf build/main.o
avr-objcopy -O ihex -R .eeprom build/main.elf build/main.hex
avr-size --format=avr --mcu=atmega328p build/main.elf

echo ""
echo "Uploading firmware..."
avrdude -p atmega328p -c arduino -P /dev/cu.usbmodem101 -b 115200 -U flash:w:build/main.hex:i

if [ $? -eq 0 ]; then
    echo ""
    echo "=================================================="
    echo "Upload successful!"
    echo "=================================================="
    echo ""
    echo "Starting serial monitor in 2 seconds..."
    echo "The fix prevents spurious characters from triggering menu loops."
    echo ""
    sleep 2
    screen /dev/cu.usbmodem101 9600
else
    echo "Upload failed!"
    exit 1
fi
