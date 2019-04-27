#include <stdio.h>

// Another useful compiler preprocessor directive is #define, which
// allows us to use a memorable name for text substitution
// in our code, which reduces duplication, as in this case
// where we use it as a constant for the array size.
// I stress that this is for TEXT substitution, since anything
// you might manually write somewhere within your code may need to be changed.
// Using this fixed text substitution avoids those difficulties.
#define ARRAY_SIZE 10    // <-- Note, no semi-colon. Important!

int main() {
  
  
  // Allocate an array of ARRAY_SIZE (i.e. 10) integers.
  // After preprocessing our code, this will simply become
  // int my_array[10];
  // if we had put a semi-colon on our #define, this would incorrectly
  // become: int my_array[10;];  -- which we can see is wrong.
  int my_array[ARRAY_SIZE];
  int i; // The loop variable

  // The for loop looks strange at first, but the three parts
  // can be understood as this:
  //   i=0;       Initialisation
  //   i<ARRAY_SIZE;  Condition on which to continue
  //   i++;           Post-loop action
  // Note that 'i++' is shorthand for: increment 'i'. This is exactly 
  // the same as a standard Java for-loop.
  for (i=0; i<ARRAY_SIZE; i++) {
    my_array[i] = i * 7;  // Stores the result in the array.
    
    // An if statement.
    if (my_array[i] <= 15) {
      printf ("%d is less than or equal to 15\n", my_array[i]);
    } else {
      printf ("%d is greater than 15\n", my_array[i]);
    }

  }
  // So now we have stored the 7 timestable in my_array.

  int index = 0;
  int value = my_array[index++];
  // Note that when we use the expression 'index++', it
  // is the pre-incremented value that is used as an array index, so this
  // statement is effectively the following two statements:
  //   int value = my_array[index];
  //   index = index + 1;
  
  // While loop is like for but with only the condition.
  while (value < 40) {
    printf("%d\n", value);
    value = my_array[index++];
  }
  //you might find it interesting to print out the value of the test
  // (value < 40). We can put this expression into printf as a parameter:
  printf("Is value < 40? %d\n",value<40);
  //Let's try with something we know is true:
  printf("3==3? Answer: %d\n",3==3);

  return 0;
}
