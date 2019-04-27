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
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
    }

  
    /* read next items for the buffer */
    charRead = readClientBuffer (clientBuffer);

    if (count + charRead < BUFFERSIZE) {  
      /* add items to the buffer */

      /* insert the characters into the buffer */
      for (i = charRead; i >=0; i--) {
	buffer[(currentIndex+i) % BUFFERSIZE]  = clientBuffer[i];
      }
  
      reg = currentIndex;
      /* now call the scheduler */
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
      printf ("Client: Returned from scheduler No. 1 \n");

      reg = (reg + charRead) % BUFFERSIZE;
      currentIndex = reg;


      reg = count;

  
      /* now call the scheduler */
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
      printf ("Client: Returned from scheduler No.2 \n");
  
      /* and continue */
      reg += charRead;
      count = reg;

      /* now call the scheduler */
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
      printf ("Client: Returned from scheduler No. 3 \n");
  
  

      printf ("Client: count = %d\n", count);
      /* now call the scheduler */
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
      printf ("Client: Returned from scheduler No.4 \n");
    
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
      /* now call the scheduler */
      nextCommand = 'S';
      write (serverSocket[0], &nextCommand, 1);
      read (serverSocket[0], &nextCommand, 1);
    }
    
    
    /* adjust the index */
    reg = currentIndex;
    /* now call the scheduler */
    nextCommand = 'S';
    write (serverSocket[0], &nextCommand, 1);
    read (serverSocket[0], &nextCommand, 1);
    
    reg = (reg - 1) % BUFFERSIZE;
    
    currentIndex = reg;
    count --;

    tmp = buffer[currentIndex];
    printf ("Server: The next element in the buffer is %d\n", tmp);


  }
  exit (0);
}
  



int main (int argc, char **argv) {

    
    pthread_t clientpid, serverpid;
    char nextProcess, nextCommand;

     
    /* create the sockets */
    if ((socketpair (AF_UNIX, SOCK_STREAM, 0, clientSocket) < 0 ) || socketpair (AF_UNIX, SOCK_STREAM, 0, serverSocket)< 0 ) {
      fprintf (stderr, "Socket creation failed!\n");
      exit (1);
    }

    /* Now create client and server process */
    if (pthread_create (&clientpid, NULL, client, NULL) == -1) { 
      fprintf (stderr, "Couldn't create client thread\n");
      exit (1);
    }
    
    /* now fork the server */
      
    if (pthread_create (&serverpid, NULL, server, NULL) == -1) { 
      fprintf (stderr, "Couldn't create server thread\n");
      exit (1);
    }
      
    /* Scheduler */
      
    while (1) {
      printf ("Scheduler: Next process to run: ");

      scanf ("%c", &nextProcess);
      /*       printf ("\nRead character %c\n", nextProcess);*/
      switch (nextProcess) {
      case 'c': 
	nextCommand = 'R';
	write (clientSocket [1], &nextCommand, 1);
	read (clientSocket[1], &nextCommand, 1);
	/* 	printf ("Now executing scheduler. \n");*/
	if (nextCommand == 'E') {
	  kill (clientpid, SIGTERM);
	  kill (serverpid, SIGTERM);
	  exit (0);
	}
	break;
      case 's':
	nextCommand = 'R';
	write (serverSocket [1], &nextCommand, 1);
	read (serverSocket[1], &nextCommand, 1);
	if (nextCommand == 'E') {
	  kill (clientpid, SIGTERM);
	  kill (serverpid, SIGTERM);
	  exit (0);
	}
	break;
      default:
	printf ("Scheduler: Entered illegal command: %c\n", nextProcess);
	break;
      }
    }
}

