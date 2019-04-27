#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "entry.h"

#define  BUFFERSIZE 10
int main (int argc, char **argv) {
  
	int procFileFd;
	struct entry_t buffer[BUFFERSIZE];
	int count = 0;
	int currentlyRead = 0;
	int i;

	if (argc != 2) {
		fprintf (stderr, "usage: %s <proc filename> \n", argv[0]);
		exit (1);
	}

	procFileFd = open (argv[1], O_RDONLY);
  
	if (procFileFd == -1) {
		fprintf (stderr, "Opening failed!\n");
		exit (1);
	}

	while (count < BUFFERSIZE * sizeof (struct entry_t)) {
		/* if (lseek (procFileFd, count, SEEK_SET) == -1) { */
		/*   fprintf (stderr, "Seek failed!\n"); */
		/*   exit (1); */
		/* }  */
	    printf ("The buffer address is %lx\n", (unsigned long) (buffer +count));
		currentlyRead = read (procFileFd, buffer + count, BUFFERSIZE * sizeof (struct entry_t) - count);
		if (currentlyRead < 0) {
			fprintf (stderr, "Reading failed! \n");
			exit (1);
		}
		count = count + currentlyRead;
		if (currentlyRead == 0) { 
			/* EOF encountered */
			break;
		}
	}
	printf ("Read buffer of size %d\n", count);
	for (i = 0; i * sizeof (struct entry_t) < count; i++) {
		printf ("Next field1 element is %d\n", buffer[i].field1);
		printf ("Next field2 element is %d\n", buffer[i].field2);
	}
	close (procFileFd);
	exit (0);

}
