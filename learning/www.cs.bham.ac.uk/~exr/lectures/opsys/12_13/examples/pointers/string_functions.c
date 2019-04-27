#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Here we have a tour of some useful string processing functions,
// some like we implemented ourselves a little earlier.

int main () {

  // strlen gives us the length of a string, which is not necessarily the
  // size of the buffer in which it sits.
  char* test_string = "hello";
  printf("'%s' has %d characters\n", test_string, (int)strlen(test_string));

  // strcmp compares two strings.  The result actually refers to the
  // difference in the strings, to facilitate string sorting, so for
  // equal string we get a 0 (zero difference).
  if (strcmp("monkey", "monkey") == 0) {
    printf("The strings are the same\n");
  }
  
  // With respect to the ASCII alphabet, we can use strcmp to tell us which
  // string is greater than another.
  if (strcmp("monkey", "hello") > 0) {
    printf("The left string is (ASCII-wise) greater than the right string\n");
  }
  
  // Likewise.
  if (strcmp("hello", "monkey") < 0) {
    printf("The left string is (ASCII-wise) less than the right string\n");
  }

  // Sometimes it is useful to copy a string, like when we do not wish
  // to alter the original, so strdup offers this functionality by returning
  // the address of a newly allocated copy of the source string.
  char *source = "hello";
  char *target = strdup(source);

  printf("target = '%s'\n", target);

  // Remember: get into the habit of freeing any dynamically allocated
  // memory once you have finished with it, to avoid memory leaks.
  free(target);

  return 0;
}
