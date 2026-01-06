#!/bin/bash
# Rebuild and upload script

cd "/Users/camal/Desktop/crash detector"

echo "Building firmware..."
avr-gcc -DF_CPU=16000000UL -Os -std=gnu99 -mmcu=atmega328p -c src/main.c -o build/main.o
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

avr-gcc -mmcu=atmega328p -o build/main.elf build/main.o
if [ $? -ne 0 ]; then
    echo "Linking failed!"
    exit 1
fi

avr-objcopy -O ihex -R .eeprom build/main.elf build/main.hex
if [ $? -ne 0 ]; then
    echo "hex conversion failed!"
    exit 1
fi

avr-size --format=avr --mcu=atmega328p build/main.elf

echo ""
echo "Firmware built successfully!"
echo "Uploading to Arduino..."
echo ""

# Kill screen session to free up the port
killall screen 2>/dev/null
sleep 1

avrdude -p atmega328p -c arduino -P /dev/cu.usbmodem101 -b 115200 -U flash:w:build/main.hex:i

echo ""
echo "Upload complete! Starting serial monitor..."
sleep 2

# Restart serial monitor
screen /dev/cu.usbmodem101 9600
