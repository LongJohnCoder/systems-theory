#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFFERSIZE 16384

int main (int argc, char **argv) {
    
    char *filename = "/dev/chardev";
    int gap, length;
    int fd;
    int result;
    char buf[BUFFERSIZE];
    int i;
    
    if (argc != 3) {
	fprintf (stderr, "Usage: write <millseconds-between-writes (1-10000)> <write-size-bytes (1-%lu)>\n", sizeof(buf));
	fprintf (stderr, "E.g.: write 1000 4096\n");
	exit (1);
    }

    
    gap  = atoi(argv[1]);
    length  = atoi(argv[2]);

    if((gap<1)||(gap>10000))
    {
	fprintf (stderr, "Invalid value for <milleseconds-between-writes>(%s)\n", argv[1]);
	exit (1);
    }

    if((length<1)||(length>sizeof(buf)))
    {
	fprintf (stderr, "Invalid value for <write-size-bytes>(%s)\n", argv[2]);
	exit (1);
    }

    printf("Sleeping %d milliseconds between writes\n", gap);
    printf("Writing  %d bytes each write()\n", length);

    fd = open (filename, O_WRONLY);
    if (fd == -1) {
	fprintf (stderr, "Could not open file %s, exiting!\n", filename);
	exit (1);
    }

    for (i = 0; i < BUFFERSIZE; i++) {
	buf[i] = i %256;
    }

    while(1)
    {
       usleep(1000*gap);
    
       result = write(fd, buf, length);
       if(result<0)
       {
          printf("The write() call returned error: %s\n", strerror(errno));
       }
       else
       {
          printf("The write() call returned %d bytes written\n", result);
       }

    }

    return 0;
}

    
    

	
