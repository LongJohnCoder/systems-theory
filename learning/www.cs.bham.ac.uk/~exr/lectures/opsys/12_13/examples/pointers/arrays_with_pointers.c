#include <stdio.h>

#define ARRAY_SIZE 10

// Swap elements of two integer arrays, though this time
// we use pointer manipulation, where the two pointers
// are expected to point to the start of the arrays that are
// to be swapped.
void swap (int *p_a, int *p_b, int no_items) {
    
  int tmp;
  int i;
  for (i=0; i<no_items; i++) {
    // Swap the values pointed to by the pointers.
    tmp = *p_a;
    *p_a = *p_b;
    *p_b = tmp;
    
    // Incremented the addresses of the pointers, ready to handle
    // their next integers.
    p_a++;
    p_b++;
  }
}

int main () {
  // Allocate two integer arrays of size 10.
  int evens[ARRAY_SIZE];
  int odds[ARRAY_SIZE];
  int i;

  /* Initialise the arrays with odd numbers and even numbers. */
  for (i = 0; i < ARRAY_SIZE; i++) {
    evens[i] = 2*i;
    odds[i] = 2*i + 1;
  }
  
  // Swap the elements.  Here we see that a reference to an array
  // is simply the address of the first item in the array, so there
  // is a close relationship between pointers and arrays, with them in most
  // respects being the same thing.
  swap(evens, odds, ARRAY_SIZE);

  /* Print the result */
  for (i = 0; i< ARRAY_SIZE; i++) {
    printf("evens[%d] = %d, odds[%d] = %d\n", i, evens[i], i, odds[i]);
  }

  return 0;
}
