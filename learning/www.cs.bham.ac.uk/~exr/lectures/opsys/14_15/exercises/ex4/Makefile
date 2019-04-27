KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

MODULES = firewallInput.ko
PROGS = testip

obj-m += firewallInput.o

all: $(MODULES) $(PROGS)

firewallInput.ko: firewallInput.c
	make -C  $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f testip

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra

testip: testip.c
	gcc -Wall -Werror -o $@ $<
