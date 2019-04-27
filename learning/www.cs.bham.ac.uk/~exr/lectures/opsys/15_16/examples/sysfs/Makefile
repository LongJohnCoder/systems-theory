KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

EXTRA_CFLAGS += -I$(PWD)
MODULES = charDeviceDriver.ko 
obj-m += charDeviceDriver.o 


all: $(MODULES) 

charDeviceDriver.ko: charDeviceDriver.c 
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f $(PROGS) *.o

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

