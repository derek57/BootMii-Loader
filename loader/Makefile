include ./starlet.mk

TARGET = loader-sym.elf
TARGET_STRIPPED = loader.elf
TARGET_BINARY = loader.bin

OBJS = start.o main.o memory.o memory_asm.o crypto.o ff.o diskio.o nand.o sdhc.o sdmmc.o seeprom.o gecko.o string.o utils.o

CFLAGS = -mbig-endian -mthumb -march=armv5t
CFLAGS += -fomit-frame-pointer -ffunction-sections
CFLAGS += -Wall -Wextra -Os -pipe
CFLAGS += -DCAN_HAZ_USBGECKO -DGECKO_SAFE -DLOADER
LDFLAGS += -Wl,-N
ASFLAGS += -D_LANGUAGE_ASSEMBLY

LDSCRIPT = loader.ld
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

