#include <stdio.h>

// In C there is no boolean type: a condition is true if
// it equals 1 and is false otherwise.
// For clarity, we can define some pre-processor aliases.
#define TRUE  1
#define FALSE 0

// It would also be possible here to define an enumeration of 
// aliases, using the enum keyword:
// enum bool {FALSE,TRUE};
// In this example, FALSE automatically takes the value 0, and TRUE 1.
// Each successive key takes the next number up, so
// enum myenum {FIRST, SECOND, THIRD}; means THIRD==2.

int main() {

  int my_flag = FALSE;  // i.e. int my_flag = 0;

  if (my_flag) {        // i.e. if (0) {
    printf("It's true!\n");
  } else {
    printf("It's false.\n");
  }

  my_flag = TRUE;       // i.e. int my_flag = 1;

  if (my_flag) {
    printf("It's true!\n");
  } else {
    printf("It's false.\n");
  }

  while (TRUE) {       // i.e. while(1) {
    printf("To infinity and beyond!\n");
  }

  // Note, you'll need to press CTRL+c to abort this
  // infinitely looping process.

  return 0;
}
