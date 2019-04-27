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
  if (!newItem) {  /* Error 1: need to check whether allocation successful */
    fprintf (stderr, "Out of memory!\n");
    exit (1);
  }
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
  char *url;
  struct ListUrl *listUrl, *tmp;

  while (argc > 0) {

    if (!(fp = fopen (argv[index], "r"))) {
      /* Error 2:  need to check whether file opening was successful */
      fprintf (stderr, "%s: cannot open file %s\n", argv[0], argv[index]);
      exit (1);
    }
    
    listUrl = NULL;
    while (getline (&line, &length, fp) != -1) {

      pos = line;
      while ((pos = strstr (pos, "http://"))) {
	/* webaddress appeared in line */
	printf ("Found webaddress %s\n", pos);
	pos += 7;
	endpos = strpbrk (pos, "/"); 
	if (endpos) {
	  /* ensure we have correctly formed URL */
	  url = malloc (endpos - pos+1);
	  /* Error 3: need to allocate space for *each* element in the list!, not only once */
	  if (!url) {
	    fprintf (stderr, "Out of memory!");
	    exit (1);
	  }
	  /* store the name */
	  url = strncpy (url, pos, (endpos - pos));
	  url[endpos-pos] = '\0';
	  /* add it to the list */
	  listUrl = addItem (listUrl, url);
	  pos = endpos;
	}
      }
      free (line); /* Need to free the line which has already been processed */
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
    
    while (listUrl) { /* need to free the list */
      tmp = listUrl->next;
      free (listUrl->url);
      free (listUrl);
      listUrl = tmp;
    }

    printf ("The file %s contains %d occurrences of URL's within Computer Science \n", argv[index], i);
    argc--;
    index++;
    fclose (fp); /* file must be closed */
  }
  exit (0); 

}

	
    
	  
