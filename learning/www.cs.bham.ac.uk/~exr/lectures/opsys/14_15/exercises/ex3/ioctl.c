#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char **argv) {
    
    char *filename = "/dev/chardev";
    int max;
    int fd;
    int result;
    
    if (argc != 2) {
	fprintf (stderr, "Usage: ioctl <newmaxmegabytes (1-16)>\n");
	fprintf (stderr, "E.g.: ioctl 8\n");
	exit (1);
    }

    max = atoi(argv[1]);

    if((max < 1)||(max>16))
    {
	fprintf (stderr, "Invalid value for <newmaxmegabytes>(%s)\n", argv[1]);
	exit (1);
    }

    max *= (1024 * 1024);

    printf("New Maximum Bytes: %d\n", max);
    
    fd = open (filename, O_RDONLY);
    if (fd == -1) {
	fprintf (stderr, "Could not open file %s, exiting!\n", filename);
	exit (1);
    }

    result = ioctl (fd, 0, max);
    printf ("The result of the ioctl() call was %d\n", result);
    
    close (fd);
    return 0;
}

    
    

	
