all:
	nasm boot_sect.asm -f bin -o boot_sect.bin
	ndisasm boot_sect.bin > boot_sect.dis
	od -t x1 -A n boot_sect.bin > boot_sect.hex

run: all
	bochs

clean:
	rm -fr *.bin *.dis *.hex
