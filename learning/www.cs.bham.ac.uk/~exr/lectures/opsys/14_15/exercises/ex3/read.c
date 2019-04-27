#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFERSIZE 16384

int main (int argc, char **argv) {
    
    char *filename = "/dev/chardev";
    int gap, length;
    int fd;
    int result;
    char buf[BUFFERSIZE];
    
    if (argc != 3) {
	fprintf (stderr, "Usage: read <millseconds-between-reads (1-10000)> <read-size-bytes (1-%lu)>\n", sizeof(buf));
	fprintf (stderr, "E.g.: read 1000 4096\n");
	exit (1);
    }

    gap  = atoi(argv[1]);
    length  = atoi(argv[2]);

    if((gap<1)||(gap>10000))
    {
	fprintf (stderr, "Invalid value for <milleseconds-between-reads>(%s)\n", argv[1]);
	exit (1);
    }

    if((length<1)||(length>sizeof(buf)))
    {
	fprintf (stderr, "Invalid value for <read-size-bytes>(%s)\n", argv[1]);
	exit (1);
    }

    printf("Sleeping %d milliseconds between reads\n", gap);
    printf("Reading %d bytes each read()\n", length);

    fd = open (filename, O_RDONLY);
    if (fd == -1) {
	fprintf (stderr, "Could not open file %s, exiting!\n", filename);
	exit (1);
    }

    while(1)
    {
       usleep(1000*gap);
    
       result = read(fd, buf, length);

       if(result<0)
       {
          printf("The read() call returned error: %s\n", strerror(errno));
       }
       else
       {
          printf("The read() call returned %d bytes read\n", result);
       }
    }

    return 0;
}

    
    

	
