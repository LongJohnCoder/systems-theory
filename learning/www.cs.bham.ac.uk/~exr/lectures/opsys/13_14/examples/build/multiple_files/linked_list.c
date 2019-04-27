#include <stdlib.h>      // Angle brackets mean, look in pre-defined
                         // search paths for include files (generally on Linux systems
                         // this is /usr/include)
#include "linked_list.h" // Quotes mean a local path to the include
                         // file.  We include this here, since we need
                         // the prototype and struct definitions in this
                         // piece of code.

List* create_list() {
  List *p_list = malloc(sizeof(List));
  p_list->p_head = NULL;
  p_list->p_tail = NULL;
  return p_list;
}

void append_list(List *p_list, void *p_data) {
  
  // Allocate some memory for the size of our structure.
  ListItem *p_new_item = malloc(sizeof(ListItem));
  p_new_item->p_previous = p_list->p_tail; // Link backwards.
  p_new_item->p_next = NULL;       // We are the new tail -> no p_next.
  p_new_item->p_data = p_data;     // Set data pointer.
 
  // If there is a tail, link it to us; else we must also be the head.
  if (p_list->p_tail) {
    p_list->p_tail->p_next = p_new_item;
  } else {
    p_list->p_head = p_new_item;
  }

  // Now we are the new tail.
  p_list->p_tail = p_new_item;
}


void free_list(List* p_list) {
  // I'll leave this for you to think about.
}
