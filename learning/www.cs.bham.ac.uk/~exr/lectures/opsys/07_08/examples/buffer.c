#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#define BUFFERSIZE 20

int buffer[BUFFERSIZE];     /* global buffer */
int currentIndex = 0;      /* currently used element */
int count = 0;             /* how many buffer elements are used */

int clientSocket[2];
int serverSocket[2];

int readClientBuffer (int clientBuffer []) {

  int index = 0;
  int tmp;
  while (index < BUFFERSIZE) {
    printf ("Client: Enter the next element: ");
    scanf ("%d", &tmp);
    if (tmp == 0) {
      return index;
    }

    clientBuffer[index] = tmp;
    index ++;
  }
  return index;
}

    
void *client () {
  
  int clientBuffer[BUFFERSIZE];

  int reg;
  char nextCommand;
  int charRead;
  int i;
  
  read (clientSocket[0], &nextCommand, 1);
  while (1) {
    while (count == BUFFERSIZE) {
      ; 
    }
  
    /* read next items for the buffer */
    charRead = readClientBuffer (clientBuffer);

    if (count + charRead < BUFFERSIZE) {  
      /* add items to the buffer */

      /* insert the characters into the buffer */
      for (i = charRead; i >=0; i--) {
	buffer[(currentIndex+i) % BUFFERSIZE]  = clientBuffer[i];
      }
  
      currentIndex = (currentIndex + charRead) % BUFFERSIZE;

      count = count + charRead;
    }
    else {
      fprintf (stderr, "Client: Too many characters read, discarding!\n");
    }
  }
}
  
void *server () {

  int reg;
  int tmp;
  char nextCommand;

  read (serverSocket[0], &nextCommand, 1);
  printf ("Server: Now executing server, count = %d\n", count);
  while (1) {
    while (count == 0) {
      ;
    }
    
    
    currentIndex = (currentIndex - 1 ) % BUFFERSIZE;
    count --;

    tmp = buffer[currentIndex];
    printf ("Server: The next element in the buffer is %d\n", tmp);

  }
  exit (0);
}
  

