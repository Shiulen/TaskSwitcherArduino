# Minimal preemptive scheduler + UART driver for Arduino Uno (ATmega328P)
# Build & flash with avr-gcc + avrdude.
#
# Usage:
#   make                # build
#   make flash PORT=/dev/ttyACM0   # upload (set PORT for your system)
#   make clean

MCU = atmega328p
F_CPU = 16000000UL
BAUD ?= 115200

PORT ?= /dev/ttyACM0
PROGRAMMER ?= arduino
# Typical Uno bootloader baudrate is 115200 (old) or 57600 (very old). Adjust if needed.
AVRDUDE_BAUD ?= 115200

TARGET = uno_os_serial
BUILD = build
SRC_DIR = src
INC_DIR = include

CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -ffunction-sections -fdata-sections -MMD -MP -Wall -Wextra -std=gnu11 -I$(INC_DIR)
ASFLAGS = $(CFLAGS)
LDFLAGS = -Wl,--gc-sections

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(filter %.c,$(SRCS))) \
       $(patsubst $(SRC_DIR)/%.S,$(BUILD)/%.o,$(filter %.S,$(SRCS)))
DEPS = $(OBJS:.o=.d)

$(BUILD)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

all: $(BUILD)/$(TARGET).hex

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	@size $(BUILD)/$(TARGET).elf

flash: $(BUILD)/$(TARGET).hex
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(AVRDUDE_BAUD) -D -U flash:w:$<:i

clean:
	rm -rf $(BUILD)

-include $(DEPS)
