PREFIX=arm-linux-gnueabihf-
CC=$(PREFIX)gcc
LD=$(PREFIX)ld
AR=$(PREFIX)ar
OBJCOPY=$(PREFIX)objcopy
OBJDUMP=$(PREFIX)objdump

CFLAGS =  -nostdinc

objs = start.o uart.o string.o init.o exception.o main.o

dep_files := $(patsubst %,.%.d, $(objs))
dep_files := $(wildcard $(dep_files))

led.img : $(objs)
	$(LD) -T imx6ull.lds -g $^ -o led.elf 
	$(OBJCOPY) -O binary -S led.elf  led.bin
	$(OBJDUMP) -D -m arm  led.elf  > led.dis	
	mkimage -n ./tools/imximage.cfg.cfgtmp -T imximage -e 0x80100000 -d led.bin led.imx
	dd if=/dev/zero of=1k.bin bs=1024 count=1
	cat 1k.bin led.imx > led.img

ifneq ($(dep_files),)
include $(dep_files)
endif

%.o : %.c
	$(PREFIX)gcc $(CFLAGS) -c -o $@ $< -MD -MF .$@.d

%.o : %.S
	$(PREFIX)gcc $(CFLAGS) -c -o $@ $< -MD -MF .$@.d

clean:
	rm -f led.dis  led.bin led.elf led.imx led.img *.o .*.d


	
