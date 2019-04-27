/*
When we break a C program up into several source files, there will then be some
dependencies among them: one piece of code may call a function defined in
another or may use a variable defined in another.

- Somehow, the compiler must know how to match up all function calls with their
  implementations when the object files ('.o' files) of each source file are
  combined into the final executable file.

- For this to happen, before the compiler sees an instruction to call a
  function it must first have seen a signature of that function (i.e. a
  prototype, detailing its return type and argument types), in the same way
  that any variables used must first have had their types defined, including
  composite types such as struct.

- If every time we wished to call a function from one source file to another we
  had to define its prototype, we would end up with a lot of duplication, and
  a big headache if we later changed the function signature.

- So, by convention, we put prototypes and general type definitions of things
  that we would like to be accessed from other code files into a header file,
  such as this one, which can then be included wherever those entities are
  required, in exactly the same way that we have already used the header files
  of some standard libraries.

*/

// Displays a string on the screen.
void display_text(char* text);

// Reads a string from the keyboard, allocating an appropriate
// amount of memory.
char* read_text();
