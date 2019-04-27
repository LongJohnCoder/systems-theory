# Examine me if you wish, but we will learn about Makefiles in a later
# lecture.  This file allows us to automate tedious repetitive build
# commands
CFLAGS = -Wall -Werror -D _GNU_SOURCE
GCC = gcc $(CFLAGS)

all: hello_world sum functions arrays_and_loops arguments boolean

hello_world: hello_world.c
	$(GCC) hello_world.c -o hello_world

sum: sum.c
	$(GCC) $< -o $@

functions: functions.c
	$(GCC) $< -o $@

arrays_and_loops: arrays_and_loops.c
	$(GCC) $< -o $@

arguments: arguments.c
	$(GCC) $< -o $@

boolean: boolean.c
	$(GCC) $< -o $@

clean:
	rm -f hello_world sum functions arguments boolean arrays_and_loops
