#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv) {

  // Do a simple check to ensure the user passed an argument - remember,
  // argv[0] is always the name of the application, so argv[1] will be
  // the file name.
  if (argc != 2) {
    fprintf(stderr, "Exactly one argument required, namely the filename\n");
    exit (1);
  }
        
  // Try to open the file, aborting the app if this fails.
  FILE *p_file;
  p_file = fopen (argv[1], "r");
  if (p_file == NULL) {
    fprintf(stderr, "%s: Couldn't open file %s!\n", argv[0], argv[1]);
    exit (1);
  }

  // Tell the user which file we are reading.
  printf("Reading file: %s\n", argv[1]);

  //we used getline in yesterday's lecture with stdin: we can instead
  //use it to read from a file pointer.
  char *line = NULL; // A string pointer to hold each file line.
  size_t len;        // We need this since getline tells us how
                     // much space it has allocated for each line.
  int line_no = 0;   // So we can print alongside each line its number.
  
  // Loop until getline fails to read a line (i.e. returns -1).
  while (getline (&line, &len, p_file) != -1) {
    printf ("Line %d is: %s", line_no, line);
    free (line); // We've done with this line, so free it.
    line = NULL; // Must set to NULL so getline will allocate mem
    line_no++;   // Increment line number, for display purposes.
  }

  // When we exit, the OS will close our file (flushing written data), but it
  // is good to get into the habit of closing files (or similar resources)
  // when you are done with them.
  fclose(p_file);

  return 0;
}
	
