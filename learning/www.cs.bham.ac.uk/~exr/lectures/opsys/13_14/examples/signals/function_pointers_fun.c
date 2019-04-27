/*
 Here we user pointer casting to trick the compiler into calling a function with
 the wrong signature, to demonstrate why the usual type checking of a compiler
 is necessary.
 */
#include <stdio.h>

#define TRUE 1
#define FALSE 0

// Our function - it doesn't matter what it does for this example.
int odd_filter(int index, int value) {
  printf("odd_filter called with (index->%d, value->%d)\n", index, value);
  if (value % 2) {
    return TRUE;
  }
  return FALSE;
}  

int main() {
  printf("Started\n");
 
  // Here we declare a pointer to the function using the correct signature (i.e.
  // take (int,int) and return int) and we point that to our function.
  int (*p_function_1)(int,int) = odd_filter;
 
  // Now we declare a pointer with a different signature (i.e. takes no args
  // and returns nothing), and since any pointer is simply an address, we can
  // cast with a void* pointer (i.e. a pointer to no specific type) to set our
  // dodgy function pointer to the actual function's address.  The compiler is
  // happy to allow pointer casts to and from void* and trusts that we know
  // what we are doing.
  void (*p_function_2)() = (void*) odd_filter;
  
  // So what happens when we call this? Args are random numbers, since when
  // the function runs it uses two int's worth of bytes from the stack, even
  // though no values were actually pushed onto the stack before the low-level
  // function call was made.
  p_function_2(); 
  
  // This shows correct args, since the compiler uses the correct function
  // signature to push args onto the stack.
  p_function_1(3,7);

  // Args are same as in last call, since they remained on the stack from the
  // last call, and nothing was put in their place.  You cannot rely on this
  // behaviour, but it gives an insight into how the variables must be pushed
  // onto the stack before the call, so they can be popped back off inside the
  // function and used as local variables.
  p_function_2();
  return 0;
}
