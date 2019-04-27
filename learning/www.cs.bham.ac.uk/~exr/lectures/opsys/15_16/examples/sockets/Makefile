# Examine me if you wish, but we will learn about Makefiles in a later
# lecture.  This file allows us to automate tedious repetitive build
# commands
CFLAGS = -Wall -Werror -pthread -D _GNU_SOURCE
GCC = gcc $(CFLAGS)


APPS = server serverThreaded client host_lookup

all: $(APPS)

server: server.c
	$(GCC) -o server server.c

serverThreaded: serverThreaded.c
	$(GCC) -o serverThreaded serverThreaded.c

client: client.c
	$(GCC) -o client client.c

host_lookup: host_lookup.c
	$(GCC) -o host_lookup host_lookup.c
clean:
	rm -f $(APPS) 
