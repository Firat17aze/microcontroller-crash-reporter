# ============================================================================
# BLACK BOX FORENSIC CRASH REPORTER - MAKEFILE
# Target: ATmega328P (Arduino Uno)
# Compiler: avr-gcc (bare metal, no Arduino IDE)
# ============================================================================

# Target MCU and Clock
MCU = atmega328p
F_CPU = 16000000UL

# Programmer settings (Arduino bootloader via USB)
PROGRAMMER = arduino
PORT = /dev/tty.usbmodem* 
# On Linux, use: /dev/ttyACM0 or /dev/ttyUSB0
# On Windows, use: COM3 (or appropriate COM port)

BAUD = 115200

# Toolchain
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AVRDUDE = avrdude

# Compiler Flags
# -mmcu: Target MCU
# -DF_CPU: Define CPU frequency for delay calculations
# -Os: Optimize for size (important for microcontrollers!)
# -Wall -Wextra: Enable all warnings
# -std=gnu99: Use GNU C99 standard
# -funsigned-char: char is unsigned by default
# -funsigned-bitfields: bitfields are unsigned by default
# -fpack-struct: Pack structures tightly
# -fshort-enums: Use smallest type for enums
CFLAGS = -mmcu=$(MCU) \
         -DF_CPU=$(F_CPU) \
         -Os \
         -Wall -Wextra \
         -std=gnu99 \
         -funsigned-char \
         -funsigned-bitfields \
         -fpack-struct \
         -fshort-enums

# Linker Flags
LDFLAGS = -mmcu=$(MCU)

# Project files
TARGET = crash_reporter
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean flash size disasm

# Default target: build the hex file
all: $(TARGET).hex size

# Compile C to object file
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to ELF
$(TARGET).elf: $(OBJECTS)
	@echo "Linking..."
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Convert ELF to Intel HEX (for programming)
$(TARGET).hex: $(TARGET).elf
	@echo "Creating HEX file..."
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Show compiled size (important for resource-constrained MCU!)
size: $(TARGET).elf
	@echo ""
	@echo "============ BUILD SIZE ============"
	$(SIZE) --format=avr --mcu=$(MCU) $(TARGET).elf
	@echo "====================================="
	@echo ""
	@echo "ATmega328P Resources:"
	@echo "  Flash:  32KB (32768 bytes)"
	@echo "  SRAM:   2KB  (2048 bytes)"
	@echo "  EEPROM: 1KB  (1024 bytes)"
	@echo ""

# Flash the program to Arduino Uno
# Uses Arduino's bootloader (no external programmer needed!)
flash: $(TARGET).hex
	@echo "Flashing to Arduino Uno..."
	@echo "Make sure Arduino is connected and port is correct!"
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

# Alternative flash for specific port (useful for debugging)
flash-port: $(TARGET).hex
	@echo "Enter port (e.g., /dev/tty.usbmodem1411):"
	@read port; \
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $$port -b $(BAUD) -U flash:w:$<:i

# Generate disassembly for debugging
disasm: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).lst
	@echo "Disassembly saved to $(TARGET).lst"

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).hex $(TARGET).lst
	@echo "Cleaned!"

# ============================================================================
# Convenience targets
# ============================================================================

# Build and flash in one step
upload: all flash

# Monitor serial output (9600 baud as set in code)
# Requires 'screen' or 'picocom' to be installed
monitor:
	@echo "Opening serial monitor at 9600 baud..."
	@echo "Press Ctrl+A then K to exit screen"
	screen $(PORT) 9600

# Alternative monitor using picocom
monitor-picocom:
	@echo "Opening serial monitor with picocom..."
	@echo "Press Ctrl+A then Ctrl+X to exit"
	picocom -b 9600 $(PORT)

# ============================================================================
# Help
# ============================================================================
help:
	@echo ""
	@echo "Black Box Forensic Crash Reporter - Build System"
	@echo "================================================="
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the project (creates .hex file)"
	@echo "  make flash    - Upload to Arduino Uno"
	@echo "  make upload   - Build and upload in one step"
	@echo "  make monitor  - Open serial monitor (9600 baud)"
	@echo "  make size     - Show memory usage"
	@echo "  make disasm   - Generate disassembly listing"
	@echo "  make clean    - Remove build artifacts"
	@echo ""
	@echo "Configuration:"
	@echo "  MCU:    $(MCU)"
	@echo "  F_CPU:  $(F_CPU)"
	@echo "  PORT:   $(PORT)"
	@echo ""
	@echo "To change port: make flash PORT=/dev/ttyACM0"
	@echo ""

