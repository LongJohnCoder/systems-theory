#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Reads a file and passes its content line by line to the kernel */
int main (int argc, char **argv) {
  
  FILE *inputFile;
  int procFileFd;
  size_t len;
  char *line = NULL;

  if (argc != 3) {
    fprintf (stderr, "Usage: %s <input file> <proc file>\n", argv[0]);
    exit (1);
  }

  inputFile = fopen (argv[1], "r"); /* open the input file for reading */
  procFileFd = open (argv[2], O_WRONLY); /* open the proc-file for writing */
  
  if (!inputFile || (procFileFd == -1)) {
    fprintf (stderr, "Opening failed!\n");
    exit (1);
  }

  while (getline (&line, &len, inputFile) != -1) {
    write (procFileFd, line, len); /* write line to kernel */
    free (line);
    line = NULL;
  }
  
  close (procFileFd); /* make sure data is properly written */
  fclose (inputFile);
  
  exit (0);

}
  

