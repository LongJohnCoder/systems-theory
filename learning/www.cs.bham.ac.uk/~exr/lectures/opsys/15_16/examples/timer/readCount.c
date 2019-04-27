#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int main (int argc, char **argv) {
    
    int fd;
    int counters[2];
    int res;
    
    fd = open("/dev/firewallCount", O_RDONLY);
    if (fd == -1) {
	exit (1);
    }

    res = read(fd, counters, 2*sizeof(int));
    if (res <= 0) {
	fprintf(stderr, "Reading failed, result = %d\n", res);
	exit (1);
    }

    
    printf ("The counters are %d and %d\n", counters[0], counters[1]);
    close (fd);
    return 0;
}
    
