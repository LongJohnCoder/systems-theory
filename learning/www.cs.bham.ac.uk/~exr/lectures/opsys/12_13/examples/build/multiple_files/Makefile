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
CFLAGS = -Wall -Werror  -D _GNU_SOURCE

# And we can use those variables like this:
GCC = gcc $(CFLAGS)

#
# Rules
#

# A rule called 'run' which has the dependency 'todo_list' (our app), and
# which when run, by typing 'make run' on the command line, will execute the
# command ./todo_list.  This is useful, since whenever I change some piece of
# code, the application will be rebuilt before running it.  Note that the body
# of the rule must be indented by a tab - not spaces!
run: todo_list
	./todo_list

# This rule builds our executable and so depends on the object files.
todo_list: todo_list.o user_interface.o linked_list.o
	gcc -Wall -Werror  -D _GNU_SOURCE -o todo_list todo_list.o user_interface.o linked_list.o

# This rule builds todo_list.o and additionally depends on the header files
# it includes, since changes to those must result in the object file being
# rebuilt
todo_list.o: todo_list.c user_interface.h linked_list.h
	gcc -Wall -Werror  -D _GNU_SOURCE -c todo_list.c -o todo_list.o

# Similar to above, though to save some typing we use variables in
# our command.  The special variable '$<' means the first dependency (i.e.
# user_interface.c), and '$@' means the rule target (i.e. user_interface.o)
user_interface.o: user_interface.c 
	$(GCC) -c $< -o $@

# Same as above.
linked_list.o: linked_list.c linked_list.h
	$(GCC) -c $< -o $@

# It is always a good idea to be able to clean your source code folder of any
# generated files, like when you wish to package the source code for someone
# else to build.  This has no dependencies, so will always execute when you
# type 'make clean'.
clean:
	rm -f *.o todo_list
