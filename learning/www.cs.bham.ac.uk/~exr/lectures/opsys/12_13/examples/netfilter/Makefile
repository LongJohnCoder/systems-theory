CFLAGS=-Wall -Werror
LIBS=-lnetfilter_queue

all: nfqnl_test

nfqnl_test: nfqnl_test.o
	$(CC) $(CFLAGS) -o "$@" $(LIBS) $<

nfqnl_test.o: nfqnl_test.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o nfqnl_test
	 
