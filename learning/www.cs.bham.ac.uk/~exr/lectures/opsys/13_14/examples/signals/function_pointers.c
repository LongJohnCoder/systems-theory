/*
 Here we look at function pointers and see an example of where they can be
 useful.  We've already seen them when we worked with pthread, since
 pthread_create declares its start_routine argument to be a pointer to a
 function of type void* some_func(void*) (i.e. any function that takes a single
 void* argument and returns a void*):

   int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                       void *(*start_routine) (void *), void *arg);

 Admittedly, the notation for declaring function pointers looks a bit strange,
 but, just as with any other type we define, the pointer must define the exact
 signature of the function it points to, so the compiler can prepare the machine
 code call exactly (e.g. push the correct number and size of bytes onto the
 stack for the expected arguments, etc.)
 */
#include <stdio.h>

#define TRUE 1
#define FALSE 0

// Note that sizeof() tells us the number of bytes occupied by a
// type/struct/array so to get the length of an array we divide its size by the
// size of the first item (e.g. int, char, etc.).  Often you see people use
// handy preprocessor macro functions like this.
#define ARRAY_LENGTH(array) sizeof(array)/sizeof(array[0])

// To demonstrate the use of pointers to functions, we allow this function to
// take as an argument a pointer to a filer function, which can be any
// function that takes two int arguments and returns an int.  If the
// particular filter function returns TRUE, then this function will print out
// a given number from the array.
void filter_numbers(int numbers[], size_t size, int (*filter_function)(int, int)) {
  int i;

  for (i=0; i<size; i++) {
    if (filter_function(i, numbers[i])) {
      printf("%d\n", numbers[i]);
    }
  }
}


int none_filter(int index, int value) {
  return TRUE;
}  

int odd_filter(int index, int value) {
  if (value % 2) { //modulus: if the remainder is odd (1), in this case
    return TRUE;
  }
  return FALSE;
}  

// Includes values that when multiplied by their array index equals 15.
int obscure_filter(int index, int value) {
  if (index * value == 15) {
    return TRUE;
  }
  return FALSE;
}

int main() {
  printf("Started\n");
  
  // Define and initialise an array of numbers. 
  int numbers[] = {2,4,1,5,16,3,23,9};

  printf("Using none_filter\n------\n");
  filter_numbers(numbers, ARRAY_LENGTH(numbers), none_filter);
  
  printf("Using odd_filter\n------\n");
  filter_numbers(numbers, ARRAY_LENGTH(numbers), odd_filter);
  
  printf("Using obscure_filter\n------\n");
  filter_numbers(numbers, ARRAY_LENGTH(numbers), obscure_filter);
  return 0;
}
