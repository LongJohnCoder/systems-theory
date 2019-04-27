#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ListUrl {
  char *url;
  struct ListUrl *next;
};

/* adds string to the list at the front */
struct ListUrl *addItem (struct ListUrl *list, char *url) {

  struct ListUrl *newItem;
  

  newItem = malloc (sizeof (struct ListUrl));
  newItem->url = url;
  newItem->next = NULL;

  if (list != NULL) {
    newItem->next = list;
  } 
  return (newItem);
}

int main (int argc, char **argv) {

  size_t length;
  int index = 1;
  int i = 0;
  FILE *fp;
  argc--;
  char *line = NULL;
  char *pos, *endpos;
  struct ListUrl *listUrl, *tmp;
  char *url;

  /* obtain the memory for the URL */
  url = malloc (10);

  while (argc > 0) {

    fp = fopen (argv[index], "r");

    listUrl = NULL;
    while (getline (&line, &length, fp) != -1) {

      printf ("The next line is %s\n", line);
      pos = line;
      while ((pos = strstr (pos, "http://"))) {
	/* webaddress appeared in line */
	printf ("Found webaddress %s\n", pos);
	pos += 7;
	endpos = strpbrk (pos, "/");
	if (endpos) {
	  /* ensure we have correctly formed URL */
	  /* store the name */
	  url = strncpy (url, pos, (endpos - pos));
	  url[endpos-pos] = '\0';
	  /* add it to the list */
	  listUrl = addItem (listUrl, url);
	  pos = endpos;
	}
      }
      line = NULL;
    }

    /* check how many times cs.bham.ac.uk is occurring */
    tmp = listUrl;
    while (tmp) {
      if (strstr (tmp->url, "cs.bham.ac.uk")) {
	i++;
      }
      tmp = tmp->next;
    }
    
    printf ("The file %s contains %d occurrences of URL's within Computer Science \n", argv[index], i);
    argc--;
    index++;
  }
  exit (0);

}

	
    
	  
