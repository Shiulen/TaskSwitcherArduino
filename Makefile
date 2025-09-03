CC       = avr-gcc
AS       = avr-gcc
OBJCOPY  = avr-objcopy
AVRDUDE  = avrdude

MCU      = atmega328p
F_CPU    = 16000000UL

PORT     = /dev/ttyACM0
BAUD     = 115200

INCLUDE_DIRS = -Iinclude
CC_OPTS = -Wall -std=gnu11 -DF_CPU=$(F_CPU) -Os -mmcu=$(MCU) $(INCLUDE_DIRS)
AS_OPTS = -x assembler-with-cpp $(CC_OPTS)

OBJS = src/main.o \
       src/scheduler.o \
       src/tcb.o \
       src/uart.o \
       src/arch.o

BIN  = uno_os_serial.elf

.PHONY: all clean flash

all: $(BIN)

%.o: %.c
	$(CC) $(CC_OPTS) -c $< -o $@

%.o: %.S
	$(AS) $(AS_OPTS) -c $< -o $@

%.elf: $(OBJS)
	$(CC) $(CC_OPTS) -o $@ $^

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(BIN:.elf=.hex)
	$(AVRDUDE) -p m328p -c arduino -P $(PORT) -b $(BAUD) -U flash:w:$<:i

clean:
	rm -f $(OBJS) $(BIN) *.hex *~ 
