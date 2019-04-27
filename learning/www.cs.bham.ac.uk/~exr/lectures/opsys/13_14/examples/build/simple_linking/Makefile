# Make performs a very simple function, which is to process /rules/ (i.e. a
# sequence of commands) which build /targets/ (e.g. files of some type) from
# other files, such that we can save typing long-winding commands and can
# re-build only those files that will be affected by changes to the source
# code - essential for large projects (e.g. the linux kernel) where
# re-building everything may take a long time.

# In practice, to improve portability of code, make is augmented with tools
# that generate make files, such as autotools:
#  http://www.gnu.org/software/hello/manual/automake/Autotools-Introduction.html
# but it is important to understand how to write a make file.  To learn more
# about make see: http://www.gnu.org/software/make/manual/

# To save typing, we can define variables in Makefiles
CFLAGS = -Wall -Werror -D _GNU_SOURCE

# And we can use those variables like this:
GCC = gcc $(CFLAGS)

# ====
# Rules
# ====

# The first target will be made by default
all: hello_world

# This rule builds our executable and so depends on the object files.
hello_world: hello_world.o more_hello_world.o
	gcc -Wall -Werror -D _GNU_SOURCE -o hello_world hello_world.o more_hello_world.o

# This rule builds hello_world.o. Since hello_world.o depends on a function defined
# in more_hello_world, we need to use the header file hellofunctions.h, so we have to add them
# as dependancies. Nore that we simply replace "-Wall -Werror ..." with our variable $(CFLAGS). 
# Again, we don't need to mention hellofunctions.c in the gcc line, as hello_world.c does so.
hello_world.o: hello_world.c hellofunctions.h
	gcc $(CFLAGS) -c hello_world.c -o hello_world.o

# Similar to above, though to save some typing we use variables in
# our command.  The special variable '$<' means the first dependency (i.e.
# user_interface.c), and '$@' means the rule target (i.e. user_interface.o)
# This time, rather than calling gcc here, we use its variable, which includes
# the reference to $(CFLAGS):
more_hello_world.o: more_hello_world.c 
	$(GCC) -c $< -o $@

# It is always a good idea to be able to clean your source code folder of any
# generated files, like when you wish to package the source code for someone
# else to build.  This has no dependencies, so will always execute when you
# type 'make clean'.
clean:
	rm -f *.o hello_world
