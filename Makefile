all: BootMii-Loader_blocks_1-4.flash

.PHONY: BootMii-Loader_blocks_1-4.flash

BootMii-Loader_blocks_1-4.flash:
	@$(MAKE) -C ./bin2c-master
	@$(MAKE) -C ./ecctool
	@$(MAKE) -C ./mini
	@./bin2c-master/bin2c ./mini/armboot.bin ./loader/armboot.h armboot
	@$(MAKE) -C ./freemem
	@./bin2c-master/bin2c ./freemem/freemem.bin ./loader/freemem.h freemem
	@$(MAKE) -C ./loader
	@./bin2c-master/bin2c ./loader/loader.bin ./preloader/preload.h preload
	@$(MAKE) -C ./preloader
	@dd conv=notrunc if=./bin/NandBoot-ECC.bin of=./BootMii-Loader_blocks_1-4.flash bs=1
	@dd conv=notrunc if=./preloader/preload.flash of=./BootMii-Loader_blocks_1-4.flash bs=1 seek=135168
	@dd conv=notrunc if=./bin/bootmii_blocks.bin of=./BootMii-Loader_blocks_1-4.flash bs=1 seek=270336

clean:
	@-rm -f ./BootMii-Loader_blocks_1-4.flash
	@-rm -f ./preloader/preload.h
	@-rm -f ./loader/armboot.h
	@-rm -f ./loader/freemem.h
	@$(MAKE) -C ./mini clean
	@$(MAKE) -C ./bin2c-master clean
	@$(MAKE) -C ./loader clean
	@$(MAKE) -C ./preloader clean
	@$(MAKE) -C ./freemem clean
	@$(MAKE) -C ./ecctool clean

