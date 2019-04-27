#
# Our top-level make file.
#

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
