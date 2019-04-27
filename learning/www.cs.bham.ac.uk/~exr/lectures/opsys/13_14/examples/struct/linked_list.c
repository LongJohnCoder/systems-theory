#include <stdio.h>
#include <stdlib.h>

#define NO_ITEMS 10
#define MAX_DATA_CHARS 30

// Define our list item as 'struct ListItem', and also
// typedef it under the same name.
typedef struct ListItem {
  char *s; // The item stored in the list
  // The struct can contain references to itself, though
  // within here the type is not yet defined, so we must use the
  // keyword 'struct'.
  struct ListItem *p_next;
} ListItem;

// Adds a new item to the beginning of a list, returning the new
// item (ie the head of the new list *)
ListItem* add_item(ListItem *p_head, char *s) {
  
  // Allocate some memory for the size of our structure.
  ListItem *p_new_item = malloc(sizeof(ListItem));
  p_new_item->p_next = p_head;       // We are the new tail.
  p_new_item->s = s;     // Set data pointer.
 
  return p_new_item;
}

int main () {

  int i;

  // The empty list is represented by a pointer to NULL.
  ListItem *p_head = NULL;

  // Now add some items onto the beginning of the list.
  for (i=0; i<NO_ITEMS; i++) {
    
    // Allocate some memory for our string, and use snprintf to
    // create a formatted string (see GNU API docs) much like printf
    // but instead writing to memory rather than the screen.
    char* s = malloc(MAX_DATA_CHARS);
    snprintf(s, (size_t) MAX_DATA_CHARS, "%d sheep\n", i);
    
    // Add s at the beginning of the list
    p_head = add_item(p_head, s);
  }

  // Now print out the list
  printf("The list is:\n");
  ListItem *p_current_item = p_head;
  while (p_current_item) {    // Loop while the current pointer is not NULL
    if (p_current_item->s) {
      printf("%s\n", p_current_item->s);
    } else {
      printf("NO DATA\n");
    }
    // Advanct the current pointer to the next item in the list.
    p_current_item = p_current_item->p_next;
  }

  // Now free all of the dynamically allocated memory: that means,
  // the ListItem structures /and/ the actual data they point to.
  printf("\nFREEING LIST\n");
  p_current_item = p_head;
  int items_freed = 0; // Just so we can see what is happening.
  while (p_current_item) {
    // Make a copy of the 'next' pointer which would otherwise
    // be lost when we free the current item.
    ListItem *p_next = p_current_item->p_next;

    // If the item pointed to data, free it.
    if (p_current_item->s) {
      free(p_current_item->s);
    }
   
    // Now free the actual list item structure.
    free(p_current_item);

    // Now advance to the next list item.
    p_current_item = p_next;
    items_freed++;
    printf("Freed %d items in total\n", items_freed);
  }

  // Study, STUDY, S-T-U-D-Y this code, since it encompasses
  // many of the complexities of C programming which if you
  // can get to grips with will really help you to understand
  // what is to follow.

  return 0;
}
