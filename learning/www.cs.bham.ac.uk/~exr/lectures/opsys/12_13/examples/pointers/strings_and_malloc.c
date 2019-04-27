#include <stdio.h>
#include <stdlib.h> // For malloc

int main () {

  // We can represent a string as a pointer to a char,
  // and in this instance we initialise the address of
  // the pointer to that of a literal string whose space
  // will be allocated at compile time.
  char *string_a = "first string";
  printf("string_a -> '%s'\n", string_a);
  
  // Alternatively, we could specify the string as an array
  // of chars, storing the literal string within it.
  char string_b[15] = "second string";
  printf("string_b -> '%s'\n", string_b);
  
  // More interestingly, we could define a pointer to
  // a char and allocate the memory (from the heap) at runtime, using
  // the malloc (i.e. memory allocation) library function.
  // malloc returns the address of the first allocated byte.
  char *string_c;
  string_c = malloc(6); // Allocate 6 bytes.
  if (string_c == NULL) { // If malloc ever returns NULL...
    perror("We are out of memory!\n");
    exit(1);
  }
  string_c[0] = 'h';
  string_c[1] = 'e';
  string_c[2] = 'l';
  string_c[3] = 'l';
  string_c[4] = 'o';
  string_c[5] = '\0';
  // Note the last character, which is zero, which is necessary
  // for determining the end of our string.  Without it printf may
  // attempt to print the whole of our processes memory until it reaches
  // a zero byte or crashes with a segmentation fault.
  // When we write a literal string, such as "first string", the compiler
  // automatically adds a terminating zero to its end.  As such 'a' is not
  // the same as "a", since the former defines a single byte translated from
  // the ASCII character and the latter allocates space for the character and
  // a terminating zero.
  printf("string_c -> '%s'\n", string_c);

  // To avoid memory leaks, we must be sure to free dynamically
  // allocated memory when we have finished with it.
  // This says: free the sequence of bytes that was allocated from
  // the address pointed to by 'string_c'.
  free(string_c);

  // Note that, in theory, your process could simply set a pointer
  // to an arbitrary memory location (e.g. int *p_someaddress = 0x23def453)
  // and you could store data there.  In practice, however, some memory
  // addresses will already contain important data that is not to be stomped
  // over, and some addresses will be non-writable (and will cause a seg fault
  // if you try to write).  So malloc() and free() keep track of
  // which addresses are being used and which are not and so help you to
  // organise your process' memory usage.

  return 0;
}
