KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

EXTRA_CFLAGS += -I$(PWD)
MODULES = charDeviceDriver.ko 
obj-m += charDeviceDriver.o 
PROGS = ioctl


all: $(MODULES)  $(PROGS)

charDeviceDriver.ko: charDeviceDriver.c  ioctl.h
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f $(PROGS) *.o

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra

ioctl: ioctl.o
	gcc -Wall -Werror -o $@ $<

ioctl.o: ioctl.c charDeviceDriver.h
	gcc -Wall -Werror -c $<
