#include <stdio.h>

int main() {
  // Define a couple of integer variables and allocate
  // their values.
  int a = 4;
  int b = 5;
  // Note that, at the machine code level, the compiler
  // will allocate some space on the stack for these, which
  // we can think of as a simple form of garbage collection, since
  // when a function returns the memory of the local variables
  // will be freed.
  // On a 32-bit CPU, each int will occupy 4 bytes (see our test of this below)

  // Note that by default number types will be treated by the compiler as signed,
  // and so will have a range of negative to positive numbers, however if we
  // wish to double the available positive range whilst using the same number of bytes,
  // we can specify a type to be unsigned, as follows:
  //   unsigned int c = <A_BIG_NUMBER>;

  // Do a calculation and store the result.
  float result = a / (float) b;
  // Note that '(float)' means cast b (an int) to a floating point
  // value, which we need to do in order to have the compiler
  // output a floating point division instruction (4 / 5.0) rather than
  // an integer division (4 / 5), which instead would yield 0.
  // You will see such casting frequently in C code.

  // Define a floating point variable.
  float x = 3.3;

  // The 'f' in 'printf' stands for 'formatted', since by using special
  // formatting sequences, we can build a formatted string from
  // one or more variable references.  Here, each '%d' displays an integer
  // string representation of each following argument; similarly '%.2f' formats
  // a floating pointer value with 2 decimal places.
  printf("%d / %d = %.2f\n", a, b, result);
  // printf is one of the few functions that takes an arbitrary
  // number of arguments.  
 
  // Let's use printf and the compiler directive sizeof() to
  // print out the size in bytes of several simple types according
  // to the CPU for which our code is being compiled
  // The size-operator needs a special format flag

  printf("There are %zu bytes in an int\n", sizeof(int));
  
  // A char is a single-byte type, so named for its use of storing
  // a single ASCII character.
  printf("There are %zu bytes in a char\n", sizeof(char));
  
  // A short usually has half the bytes of an int.  On
  // a 32-bit CPU a short would be two bytes long.
  printf("There are %zu bytes in a short\n", sizeof(short));
  
  printf("There are %zu bytes in x (a float)\n", sizeof(x));
  // 'sizeof' is very useful when we need to dynamically allocate
  // memory for certain types, which we will look at in a later
  // lecture.

  // We could format a value as hex too, which is often useful
  // for examining non-printable characters.
  printf("a * b as hex = %x\n", a * b);

  return 0;
}
