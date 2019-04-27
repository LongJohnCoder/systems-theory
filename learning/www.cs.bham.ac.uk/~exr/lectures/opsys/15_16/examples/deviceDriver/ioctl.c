#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "ioctl.h"

int main (int argc, char **argv) {
    
    char *filename; /* the name of the device */
    int fd; /* device file descriptor */
    int result;
    
    if (argc != 2) {
	fprintf (stderr, "Exactly one argument required, exiting!\n");
	exit (1);
    }

    /* ioctl  can be performed only on opened device */
    filename = argv[1];
    fd = open (filename, O_RDONLY);
    if (fd == -1) {
	fprintf (stderr, "Could not open file %s, exiting!\n", filename);
	exit (1);
    }

    result = ioctl (fd, RESET_COUNTER, 0);
    printf ("The result of the ioctl is %d\n", result);
    
    close (fd);
    return 0;
}

    
    

	
