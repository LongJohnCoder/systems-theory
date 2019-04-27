#include <stdio.h>

// Swap two integers that are passed by reference.
// It is often a good idea to follow some convention when
// naming pointers if it is not otherwise clear, so you don't
// forget you are working with a pointer.  Here use a prefix 'p_'.  
void swap_good(int *p_a, int *p_b) {
  int tmp;

  // Swap the values pointed to by the pointers.
  tmp = *p_a;
  *p_a= *p_b;
  *p_b = tmp;
}

// This does not work, since it swaps only the copied
// arguments - it has no external affect on the origins
// of those arguments
void swap_bad(int a, int b) {
  int tmp;
  tmp = a;
  a= b;
  b = tmp;
}

int main () {
  // Define and assign two integers.
  int a = 1;
  int b = 2;

  // Now they are swapped
  printf ("initially -> a = %d, b = %d\n", a, b);
  
  // Pass their *addresses* to the swap function.
  swap_good(&a, &b);

  // Now they are swapped
  printf ("swap_good -> a = %d, b = %d\n", a, b);

  // Will this swap them back? I doubt it.
  swap_bad(a, b);
  printf ("swap_bad -> a = %d, b = %d\n", a, b);

  return 0;
}
