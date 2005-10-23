# Makefile for AVR project Motor Controller
# $Id$

SRCS		= motor.c
OBJS		= ${SRCS:.c=.o}
TARGET		= motor

CC			= avr-gcc
LD			= avr-gcc
LDFLAGS		= -mmcu=atmega32
OBJCOPY		= avr-objcopy
HEX_FORMAT	= ihex
CFLAGS		= -D__AVR_ATmega32__ -mmcu=atmega32 -Wall -O2
TARGET_ELF	= $(TARGET).elf
TARGET_HEX	= $(TARGET).hex

$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) --target $(HEX_FORMAT) -R .eeprom $(TARGET_ELF) $(TARGET_HEX)

prog: $(TARGET_HEX)
	python ../spi/avrprog.py $(TARGET_HEX)

$(TARGET_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET_ELF) $(OBJS)

`$(CC) -MM $(SRCS)`:

clean:
	rm -rf $(TARGET_ELF) $(TARGET_HEX) $(OBJS)
