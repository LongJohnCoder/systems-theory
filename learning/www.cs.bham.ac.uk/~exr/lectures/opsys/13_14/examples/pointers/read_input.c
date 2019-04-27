#include <stdio.h>
#include <stdlib.h> // For free() and exit()

int main () {
   int x = 3;
  
  // In kind of the opposite way that prinf works, we
  // can use 'scanf' to read formatted input from the
  // keyboard into the specified variables, whose *addresses*
  // are passed as arguments.  Here we are reading an
  // integer, but could use a more complex format string.
  printf("Enter a number for scanf() to read: ");
  if (scanf("%d", &x)) {
    printf("x = %d\n", x);
  } else {
    printf("Input was badly formatted!\n");
    exit(1);
  }

  //For reading raw keyboard input of arbitrary length, we can use getline().
  size_t buffer_size = 0;  // To store the allocated buffer size.
  char *input = NULL;   // Our string pointer.  By initialising
                        // to point to NULL (address zero), getline()
                        // will allocate the string memory for us.
  printf("Now enter some raw text for getline() to read: \n");
  getchar (); // skip new line
  getline(&input, &buffer_size, stdin); // stdin is our processes input stream
                                        // which by default is the keyboard.

  printf("%d bytes were allocated to store the text: '%s'", (int)buffer_size, input);
  // Note that getline also includes the newline character, '\n', immediately
  // before the NULL terminator character.
 
  free(input);  // Release the memory allocated by getline().

  return 0;
}
