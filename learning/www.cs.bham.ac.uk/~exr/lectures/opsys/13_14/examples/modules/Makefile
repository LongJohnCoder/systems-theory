KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

MODULES = helloWorld.ko kernelWrite.ko kernelRead.ko firewallExtension.ko firewallExtensionUDP.ko myTimer.ko

obj-m += helloWorld.o kernelWrite.o kernelRead.o firewallExtension.o firewallExtensionUDP.o myTimer.o

all:
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra
