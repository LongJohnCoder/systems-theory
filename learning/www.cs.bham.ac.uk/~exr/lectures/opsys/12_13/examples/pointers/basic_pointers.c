#include <stdio.h>

int main() {
  
  // Allocate three integer variables.
  int x, y, z;
  // Allocate a _pointer to_ an integer, which can hold a memory
  // address of an integer (i.e. it can point to an integer type)
  int *p;

  // Assign values to our variables.
  x = 4;
  y = 6;
  z = 8;

  // Now set our integer pointer not to the value of x (i.e. 4) but
  // to the memory address of x, hence the '&' sign.
  p = &x;

  // If we print out the value of the pointer, we will now
  // see the memory address of x, which until now the compiler
  // has kept hidden from us.  This address will fall somewhere
  // on our process' stack.
  // It is much easier to interpret addresses in hexdecimal, so
  // here we use printf to format it accordingly, though printf
  // expects an integer, so we must cast our pointer.  This cast
  // works because an integer and an address both contain the same
  // number of bytes (4 on a 32-bit CPU).
  // Note that this won't work on a 64-bit CPU - use %p instead, and
  // don't do the cast.
  printf("The address x is %x\n",(int)p);
  
  // It follows that we could also print a variable's address like this.
  printf("The address y is %d\n",(int)&y);
  printf("The address z is %d\n",(int)&z);
  
  // Complimentary to the '&' notation, we can use a '*' to
  // access and manipulate the value pointed at by our pointer.
  *p = *p + 1;
  // This says: take the contents of the address pointed to by 'p',
  // add one to it, then store back into the address.
  // So we are incrementing variable 'x'.

  // Note how we have used the pointer to change the value of
  // what it points to (i.e. variable x).
  printf("The value of x now is %d\n", x);

  // ------------------------------------------------------------
  // Pointer Arithmetic
  // ------------------------------------------------------------

  // Since a pointer is a variable that holds an address, we
  // may manipulate that address as any other number.
  // Here we decrement 1 from the address within 'p', which the compiler translates
  // to: decrement the current address by the size of the pointer's type (an int, and
  // so decrement the address by 4 bytes).
  p -= 1;
  printf("p is now %x\n", (int) p);
  printf("The value pointed to by p is now %d\n", *p);
  // Strangely enough, we find that our pointer now points
  // to (the contents of) variable 'y', the number 6.
  // So through our ability in C to manipulate memory addresses,
  // we have actually revealed here that the stack as implemented by the CPU
  // grows downwards as local variables are defined in our function (i.e. 'y'
  // is at a lower memory address than 'x').
  // Usually this is the case, but will vary among operating systems and CPU
  // architectures.

  // ------------------------------------------------------------
  // So why are pointers useful?
  // ------------------------------------------------------------
  // - We can access memory directly, which is often essential for
  //   manipulating devices in the operating system kernel.
  // - We can pass arguments to functions by reference, avoiding
  //   inefficient copying of values and allowing a function to change
  //   the values of outer variables.
  // - We can also have pointers to functions, so we can pass around a function
  //   as though it is a variable.  Indeed, a function is simply a reference to
  //   the address of a chunk of code (i.e. the function's body).

  return 0;
}
