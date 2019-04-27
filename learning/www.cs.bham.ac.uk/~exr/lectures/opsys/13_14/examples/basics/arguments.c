#include <stdio.h>
#include <stdlib.h> // Contains the exit() function


// By specifying these arguments in our main function, the
// OS knows that we would like to receive an array of
// command-line arguments so that we might alter the behaviour
// of our program in some way.
//  - 'argc' is the number of arguments our program was passed
//  - 'argv' is an array (so named, a vector) of strings that
//    represent the arguments (char* will be discussed in another lecture)
int main(int argc, char* argv[]) {
  int i; // The loop variable 
    
  // List the arguments
  for (i=0; i<argc; i++) {
    printf("Argument %d is '%s'\n", i, argv[i]);
  }
  // Note that the first argument is always the name of the application
  // itself and is the only way out code can know the name of its own
  // executable file.

  // One you have access to the arguments, you can decide how they will
  // affect your program.  If you published your application, you would
  // then be expected to document the effect of all arguments, usually by
  // writing a 'man' page.

  // Simple argument check.
  if (argc < 2) {
    // All command-line processes have two output streams:
    //  stdout: for printing general info to the user
    //  stderr: for printing error messages
    // This allows, say, all error messages to be written to
    // a log file or email to the admin user.
    // 'perror' simply prints a string to the error stream.
    perror("ERROR: You must supply at least one argument!");
    // 'exit' allows us to exit the application from anywhere in the
    // code, passing an error code number to return to the OS.
    exit(1);  // We use an error code of 1 (i.e. non-zero)
    // Since we are in the main function, the same effect would
    // be had by issuing 'return 1'.
  }
  
  return 0;
}
