#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_interface.h"

void display_text(char* text) {
  printf("%s",text);
}

char* read_text() {
  char* input_line = NULL;
  size_t buffer_length;
  printf("> ");  // Print a prompt.
  int result = getline(&input_line, &buffer_length, stdin);
  if (result == -1) {
    perror("Failed to read a line\n");
    exit(1);
  }
  
  // Strip the newline char that getline includes.
  input_line[strlen(input_line)-1] = '\0';
  return input_line;
}
