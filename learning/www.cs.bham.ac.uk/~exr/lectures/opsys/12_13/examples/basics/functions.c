#include <stdio.h>

// Function that adds two integers and returns the result. Note that we don't need
// public/private, as C isn't object-oriented, so access isn't an issue. We *could* use the keyword
// 'static' to limit the visibility of a function to inside this file.
int add(int a, int b) {
  return a + b;
}

// Since we define 'divide' after it is referenced in 'main'
// we must provide the compiler with a "prototype" of the function,
// so it knows, by the time it compiles 'main', the type of the function
// (i.e. the function's input and output types). If we hadn't defined 'add' 
// already, we would have to provide a prototype for that, too. Try it out.
// A prototype is simply the function's signature, which excludes the body,
// and it is these protoypes that we find in header files of libraries.
// In fact one reason for using header files in C is that ALL the prototypes
// get included right at the top of the code, so then we do not need to
// worry about any special ordering of our function definitions.

//Prototypes in fact don't have to include the parameter names - just the types. 
//Writing 
//  float divide(int,int);
//would be equally valid. The function itself will need the parameter names
//of course!
float divide(int a, int b);   // <-- Note the semi-colon

int main() {
  printf("3 + 4 = %d\n", add(3,4));
  printf("3 / 4 = %.2f\n", divide(3,4));
  return 0;
}

// Function that performs floating point division of the integers.
float divide(int a, int b) {
  return a / (float) b;
}
