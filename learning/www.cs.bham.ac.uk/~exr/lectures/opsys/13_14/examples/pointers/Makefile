# Examine me if you wish, but we will learn about Makefiles in a later
# lecture.  This file allows us to automate tedious repetitive build
# commands
CFLAGS = -Wall -Werror -D _GNU_SOURCE
GCC = gcc $(CFLAGS)

C_FILES = $(wildcard *.c)
APPS = $(patsubst %.c,%,$(C_FILES))

all: $(APPS)

# XXX: Only works once apps exist.
run_%: %; ./$<

clean:
	rm -f $(APPS)
