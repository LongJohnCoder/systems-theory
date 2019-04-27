#include <stdio.h>
#include <stdlib.h>

int main() {
    // Create a pointer to a FILE type, which represents an abstract character
    // stream that can be read or written. Note that the pointer isn't just 
    // "the file" - it contains useful information about the file, such as where
    // the current character position is in the buffer, if it's being read or written,
    // if there are any errors (see the function "ferror(FILE *)"), or if we're
    // at the end of the file (see the function "feof(FILE *)").
    FILE *p_file;

    // Open the file 'data' in read mode ("r"), storing a reference
    // to its stream in p_file.
    p_file = fopen("data", "r");
    // Writing is similar, so have a look through the GNU C API.
    // We could also append ("a").

    // If the stream is NULL, fopen must have failed.
    if (p_file == NULL) {
        perror("Couldn't open file! Check it exists and that you have read access.\n");
        exit(1);
    }

    // Now try to read from the file's stream, which we have contrived to
    // contain the content '42,1234'.  fscanf is like scanf only it allows
    // an arbitrary stream to be read, so we can use it, say, to read formatted
    // strings from a file stream. Note that we can use fscanf to read in a file
    // and automatically parse it according to a delimiter of our choice (',' and
    // then ':' here).
    // Whereas printf's parameters after the format string were variables to include,
    // fscanf's parameters are the locations in which to store what is extracted from
    // the file.
    int value_1;
    int value_2;
    int value_3;
    int result = fscanf(p_file, "%d,%d:%d", &value_1, &value_2, &value_3);
    if (result == 0) {  // like scanf, fscanf will return zero on fail.
        perror("Couldn't read data!\n");
        exit (1);
    }

    printf ("value_1 -> %d\n", value_1);
    printf ("value_2 -> %d\n", value_2);
    printf ("value_3 -> %d\n", value_3);

    return 0;
}

