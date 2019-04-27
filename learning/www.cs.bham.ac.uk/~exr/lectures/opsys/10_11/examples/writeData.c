#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main (int argc, char **argv) {
  
  FILE *inputFile;
  int procFileFd;
  size_t len;
  char *line = NULL;

  if (argc != 3) {
    fprintf (stderr, "Exactly two arguments required!\n");
    exit (1);
  }

  inputFile = fopen (argv[1], "r");
  procFileFd = open (argv[2], O_WRONLY);
  
  if (!inputFile || (procFileFd == -1)) {
    fprintf (stderr, "Opening failed!\n");
    exit (1);
  }

  while (getline (&line, &len, inputFile) != -1) {
    write (procFileFd, line, len);
    free (line);
    line = NULL;
  }
  
  close (procFileFd);
  fclose (inputFile);
  
  exit (0);

}
  

