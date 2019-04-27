/* 
Multiline comments can go between these
star-slashes.
*/

// Single-line comments can go after a double slash.

/*
#include is a pre-processor command that simple gets replaced by the contents
of the specified file before compilation proper.  So this includes the header
file of the stdio library, which defines prototypes (functions and their
argument types) of I/O routines.  Such prototypes are needed so that the
compiler can check we are using the correctly typed arguments for our call to
stdio's printf function below. Try without this include statement.

Note that you could put the statement anywhere, but its scope will only be *below*
where it is specified. It makes considerable sense to put it at the top. 
*/  
#include <stdio.h>

// main is a special entry function that the OS looks for in our compiled
// code, allowing execution to be started from within this function.
// The main function MUST return an integer which the OS interprets as a
// completion status - a value of zero means we exited normally, and other
// values may be defined by the programmer to reflect different termination
// errors.  Note that 'int' tells the compiler that this function will return
// an integer.
// Also note that, for backwards compatibility, the function declaration 
//   int main(void){ 
// is valid, and equivalent.
int main() {
  printf("Hello World\n");  // Print a string to the screen, followed by a
                            // newline. Try this without the \n.
  return 0;                 // Return 0 to the OS, so it knows we completed
                            // normally.
  // Note how statements end with a semi-colon - you will get strange
  // compiler errors if you omit these as the compiler tries to understand
  // your code.
}
