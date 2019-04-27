KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

PROGS = readCount

MODULES = timer.ko 

obj-m += timer.o 

all: $(PROGS) $(MODULES)

timer.ko: timer.c
	make -C  $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f readCount

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

readCount: readCount.c 
	gcc -Wall -Werror -o $@ $<

