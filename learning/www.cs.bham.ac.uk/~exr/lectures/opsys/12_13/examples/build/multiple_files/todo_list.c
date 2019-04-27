#include <string.h>
#include "user_interface.h"
#include "linked_list.h"

int main() {
  
  display_text("TODO List\n========\n\n(type 'exit' to quit)\n");

  char *user_input = read_text();
  List *p_list = create_list();

  while (strcmp(user_input, "exit") != 0) {
    append_list(p_list, (void*) user_input);
    user_input = read_text();
  }

  // Now print the list.
  ListItem *p_item = p_list->p_head;
  while (p_item) {
    display_text((char*) p_item->p_data);
    display_text("\n"); // So each item displayed on new line.
    p_item = p_item->p_next;
  }
  
  // Free all memory used by the list.
  free_list(p_list);

  return 0;
}
