# $Id$

CC			= sdcc
MAIN_SOURCE	= ledboard.c
MAIN_OBJECT = ${MAIN_SOURCE:.c=.o}
SOURCES		= sdcc_bootldr.c i2c_slave.c
OBJECTS		= ${SOURCES:.c=.o}
HEADERS		= i2c_slave.h
TARGET		= ${MAIN_SOURCE:.c=.hex}
TARGET_ARCH	= pic16
TARGET_CPU	= 18f2550
OPT_FLAGS	= --fstack --optimize-df --optimize-cmp --optimize-goto --peep-asm --opt-code-size --obanksel=2
TARGET_FLAGS= -m$(TARGET_ARCH) -p$(TARGET_CPU) 
CFLAGS		= $(TARGET_FLAGS) $(OPT_FLAGS) --ivt-loc=0x800
LDFLAGS		= $(TARGET_FLAGS) $(OPT_FLAGS) -Wl "-s ./18f2550-btldr.lkr" -llibc18f.lib
LD			= sdcc

all:		$(TARGET)

prog:		$(TARGET)
	./writepic $(TARGET)

$(TARGET):	$(MAIN_OBJECT) $(OBJECTS)
	$(LD) $(MAIN_OBJECT)  $(LDFLAGS) $(OBJECTS)

clean:
	rm -rf $(TARGET) *.o *.asm *.lst *.cod
