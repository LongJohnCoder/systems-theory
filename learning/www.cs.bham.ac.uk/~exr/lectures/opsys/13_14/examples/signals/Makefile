# Define a debug macro, so we compile-in debug functionality in the macro
# examples.
DEBUG_FLAGS = -D DEBUG
CFLAGS = -Wall -Werror -D _GNU_SOURCE -g $(DEBUG_FLAGS)
GCC = gcc $(CFLAGS)

C_FILES = $(wildcard *.c)
APPS = $(patsubst %.c,%,$(C_FILES))

all: $(APPS)

# XXX: Only works once apps exist, so first run 'make all'
run_%: %; ./$<

# Generate the preprocessed source - so we can see what happens to all thos
# emacros we write
%.cpp: %.c
	$(GCC) -E $< -o $@

run_macros_1: macros_1; ./$<

clean:
	rm -f $(APPS) *.cpp
