#include <stdio.h>

#define TRUE 1
#define FALSE 0

// Computes the length of a string.  Note that our declaring
// the argument here with the keyword 'const' is simply a convenience
// to stop us from accidentally modifying things we do not want to.  In this case,
// we are asking the compiler to warn us if we use p_string to change
// the contents of the string in any way.  Here it is unlikely that we
// will make such a mistake, but in a more complex function with many
// variables using const can be a life-saver - and it is good practice.
int length_of_string(const char *p_string) {
  int length = 0;
  while (*p_string) {  // This loops until a zero character is encoutered
    length++;
    p_string++; // Now increment our pointer to addr. of next char.
  }
  return length;
}

// Returns TRUE if two strings are equal in characters; FALSE otherwise.
int strings_are_equal(const char *string_a, const char *string_b) {
  
  // If they are different lengths, they cannot be equal.
  if (length_of_string(string_a) != length_of_string(string_b)) {
    return FALSE;
  }

  // From here we can assume the strings are of equal length.
  
  // Loop until we reach the null char of one string.
  while (*string_a) {
    // If the current chars are different, strings must be different. 
    if (*string_a != *string_b) {
      return FALSE;
    }
    string_a++;
    string_b++;
  }

  // All chars must be the same.
  return TRUE;
}

// A useful wrapper function that prints a message on the equality
// of two strings
void test(const char *string_a, const char *string_b) {
  if (strings_are_equal(string_a, string_b)) {
    printf("They are the same: '%s' == '%s'\n", string_a, string_b);
  } else {
    printf("They are not the same: '%s' != '%s'\n", string_a, string_b);
  }
}

int main () {
  char *test_string = "Birmingham";

  // Test length_of_string()
  printf("'%s' has length %d\n", test_string, length_of_string(test_string));

  // Compare some strings.
  test("hello", "hello");
  test("monkey", "hello");
  test("chips", "hello");
  return (0);
}
