#include <stdio.h>

#define ARRAY_SIZE 10

// Swap elements of two integer arrays - note the empty square
// brackets, which simply tell the compiler that this function
// will accept arrays rather than single types.
void swap (int array_1[], int array_2[], int no_items) {
  int tmp;
  int i;
  
  for (i=0; i<no_items; i++) {
    tmp = array_1[i];
    array_1[i] = array_2[i];
    array_2[i] = tmp;
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
  
  /* swap the elements */
  swap(evens, odds, ARRAY_SIZE);

  /* print the result */
  for (i = 0; i< ARRAY_SIZE; i++) {
    printf("evens[%d] = %d, odds[%d] = %d\n", i, evens[i], i, odds[i]);
  }

  return 0;
}
