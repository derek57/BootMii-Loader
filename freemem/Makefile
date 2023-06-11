include ./starlet.mk

TARGET = freemem-sym.elf
TARGET_STRIPPED = freemem.elf
TARGET_BINARY = freemem.bin

OBJS = start.o main.o gecko.o utils.o

CFLAGS = -mbig-endian -mthumb -march=armv5t
CFLAGS += -fomit-frame-pointer -ffunction-sections
CFLAGS += -Wall -Wextra -Os -pipe
CFLAGS += -DCAN_HAZ_USBGECKO -DGECKO_SAFE -DLOADER
LDFLAGS += -Wl,-N
ASFLAGS += -D_LANGUAGE_ASSEMBLY

LDSCRIPT = freemem.ld
LIBS = -lgcc

include ./common.mk

all: $(TARGET_STRIPPED)

$(TARGET_STRIPPED): $(TARGET)
	@echo "  STRIP     $@"
	@cp $< $@
	@$(STRIP) -R .comment -R .ARM.attributes -s $@
	@echo "  OBJCOPY   $(TARGET_BINARY)"
	@$(OBJCOPY) -O binary $(TARGET_STRIPPED) $(TARGET_BINARY)

clean: myclean

myclean:
	@-rm -f $(TARGET_STRIPPED) $(TARGET_BINARY)

