# Examine me if you wish, but we will learn about Makefiles in a later
# lecture.  This file allows us to automate tedious repetitive build
# commands
CFLAGS = -Wall -Werror -D _GNU_SOURCE
GCC = gcc $(CFLAGS)

all: exec fork threadtest threadtestInt

exec: exec.c
	$(GCC) exec.c -o exec

fork: fork.c
	$(GCC) $< -o $@

threadtest: threadtest.c
	$(GCC) -pthread $< -o $@

threadtestInt: threadtestInt.c
	$(GCC)  -pthread -o $@ $<


clean:
	rm -f exec fork threadtest  threadtestInt 
