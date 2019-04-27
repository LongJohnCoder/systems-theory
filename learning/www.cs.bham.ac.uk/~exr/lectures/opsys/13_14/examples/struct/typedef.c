#include <stdio.h>
#include <stdlib.h>

// To save us from always having to type 'struct SomeStruct'
// whenever we delare a struct type (e.g. a variable, an argument)
// we can use 'typedef', which instructs the compiler to define
// our own custom type.
// Many of the standard libraries define their own types.  For example,
// if I stop compilation of this code immediately after pre-processing
// using the command 'gcc -E -c typedef.c -o typedef.cpp'), then look in my
// pre-processed / source code (typedef.cpp), I will see that the type
// 'size_t' is defined as:
//   typedef unsigned int size_t;
// which means it is short-hand for declaring a number that has only a
// positive range, presumably since it makes no sense to have negative
// numbers when we are talking about, say, the size of string or an amount
// of memory.
typedef struct {
  char* name;
  char* surname;
  int   age;
} Person;

// typedef is useful also for simple types.  For example, there is no boolean
// type in C, but we could define one as follows.  Here I use a char, since it
// takes up the least space of the other types and we require only to store a 1
// or 0.
typedef char boolean;
#define TRUE 1
#define FALSE 0

// Utility function to print a person's details to the screen.
void display_person(Person* person) {
  printf("%s %s is %d years old.\n", person->name, person->surname, person->age);
}

int main () {
  // Allocate a Person on the stack, initialise the attributes, then print it.
  Person person1;
  person1.name = "Ronald"; 
  person1.surname = "Laing";
  person1.age = 42;
  display_person(&person1);

  // Allocate a Person dynamically (i.e. on the heap), initialise the
  // attributes, then print it.
  Person *p_person2;
  p_person2 = malloc(sizeof(Person));
  p_person2->name = "Albert";
  p_person2->surname = "Camus";
  p_person2->age = 45;
  display_person(p_person2);

  // Don't forget to free any dynamically allocated memory.
  free(p_person2);

  // And to demonstrate our boolean type:
  boolean my_flag = TRUE;
  if (my_flag) {
    printf("my_flag -> TRUE\n");
  } else {
    printf("my_flag -> FALSE\n");
  }
  return 0;
}
