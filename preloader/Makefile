include ./starlet.mk

TARGET = preload-sym.elf
TARGET_STRIPPED = preload.elf
TARGET_BINARY = preload.bin
TARGET_FLASH = preload.flash

ECC = ../ecctool/ecc

OBJS = start.o main.o gecko.o string.o utils.o

CFLAGS = -mbig-endian -mthumb -march=armv5t
CFLAGS += -fomit-frame-pointer -ffunction-sections
CFLAGS += -Wall -Wextra -Os -pipe
CFLAGS += -DCAN_HAZ_USBGECKO -DGECKO_SAFE -DLOADER
LDFLAGS += -Wl,-N
ASFLAGS += -D_LANGUAGE_ASSEMBLY

LDSCRIPT = preload.ld
LIBS = -lgcc

include ./common.mk

all: $(TARGET_STRIPPED)

$(TARGET_STRIPPED): $(TARGET)
	@echo "  STRIP     $@"
	@cp $< $@
	@$(STRIP) -R .comment -R .ARM.attributes -s $@
	@echo "  OBJCOPY   $(TARGET_BINARY)"
	@$(OBJCOPY) -O binary $(TARGET_STRIPPED) $(TARGET_BINARY)
	@echo "  FLASH     $(TARGET_FLASH)"
	@$(ECC) $(TARGET_BINARY) $(TARGET_FLASH)


clean: myclean

myclean:
	@-rm -f $(TARGET_STRIPPED) $(TARGET_BINARY) $(TARGET_FLASH)

