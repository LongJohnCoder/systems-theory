/*
 So far we have seen how we can use the preprocessor for including files and
 define labels for code substitutions, but it is actually more powerful than
 that. 
 */
#include <stdio.h>
#include <stdlib.h>

// The preprocessor expands macros (i.e. labels) recursively, so we could write
// this, in this order, and it would resolve X -> 4
#define X VALUE
#define VALUE 4

// And the value of our macro can be any piece of code, like this
#define CONDITION (a==2 && b==4)

// And even better, we can give arguments to macros (which explains why I called
// it a macro earlier).
#define LOOP(counter, max_count) for(counter=0; counter<max_count; counter++)
// Though the arguments are simple code that is to be substituted into the value
// of our macro, which means there is no type checking or anything -> if your
// macro expands to something meaningless, your code will not compile after the
// preprocessing stage.

// Here is a good example of where a macro can be better than a C function.
// Firstly, if a macro DEBUG has been defined (e.g. passed to the compiler) then
// the ifdef-else-endif directives will ensure that our assertion code is
// substituted, otherwise we expand the ASSERT macro to nothing.
// As for the ASSERT macro, there are a few interesting points:
//  - we write !(condition) rather than !condition to ensure the ! is applied to
//  the full condition, not just the left hand side, since we do not know what
//  the condition will be in advance
//  - Next, macros must be declared on a single line, though we can break that
//  line with '\' (line continuation character) which should be followed immediately
//  by a newline
//  - Next we use #condition which encloses the argument in quotes as though it
//  were written in the code as a literal string, so we can print out the
//  condition code that was passed to the macro - very handy
//  - Note that if we write "string1" "string2" "string3" (i.e. literal strings
//  side-by-side) the compiler treats them as the string "string1string2string3"
//  so this is how our macro builds up the display string.
#ifdef DEBUG
  #define ASSERT(condition) \
    if (!(condition)) {\
      error("Assertion failed <" #condition ">\n");\
    }
#else
  #define ASSERT(condition) // Do nothing
#endif

// And finally, with macros it is possible to have functions such as a debug
// logger that can be completely compiled out (for optimisation) after debugging
// has been done.  Here we see:
// - a macro with a variable number of arguments (so we can make use of printf),
// where args will expand to the remaining comma-separated arguments passed to
// the macro, through declaration with '...'
// - the use of special macros __FILE__ and __LINE__, which expand to the
// current filename and line number -> very useful for locating the source of
// debug messages from a log file or screen dump.
#ifdef DEBUG
  #define D(format_string, args...) printf("%s:%d " format_string,__FILE__,__LINE__,args)
#else
  #define D(format_string, args...) // Do Nothing
#endif


// Just space things out so we can see the different outputs.
#define PAD_OUTPUT printf("\n\n******************\n\n")


void error(char* error_message) {
  printf("%s",error_message);
  exit(1);
}



int main() {
  printf("X expands to %d\n", X);

  int a = 2;
  int b = 4;
  int i;

  if CONDITION {   // <-- this is replaced with code before compilation.
    printf("The condition is TRUE\n");
  } else {
    printf("The condition is FALSE\n");
  }

  PAD_OUTPUT;

  LOOP(i, 10) {
    printf("Counting -> %d\n", i);
  }
  
  PAD_OUTPUT;

  ASSERT(a == 2);
  ASSERT(3 * a == 6);
 
  PAD_OUTPUT;
  
  D("This is a debug message.  Current state -> (a->%d,b->%d)\n", a, b);

  PAD_OUTPUT;

  ASSERT(a == 4 && b == 24);
  return 0;
}
