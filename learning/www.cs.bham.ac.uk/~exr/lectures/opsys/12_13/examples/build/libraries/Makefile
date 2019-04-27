#
# Our top-level make file.
#

# This means, switch to folder 'src', then run 'make run'.
run: all
	make -C src run

# Likewise, build all in the library and application.
all: 
	make -C libs/linked_list/src all
	make -C src all

# Often there will be an 'install' for installing the compiled
# application and any libraries to the user's system
install: all
	This is where you might install your files to the system for
	real use

# Clean all.
clean:
	make -C libs/linked_list/src clean
	make -C src clean
