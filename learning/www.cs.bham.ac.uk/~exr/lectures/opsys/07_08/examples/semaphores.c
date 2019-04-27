#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#define BUFFERSIZE 20
#define CLIENT 1
#define SERVER 2

int semaphore = 1;

int buffer[BUFFERSIZE];     /* global buffer */
int currentIndex = 0;      /* currently used element */
int count = 0;             /* how many buffer elements are used */

int clientSocket[2];
int serverSocket[2];

void acquire (int process) {
  char nextCommand;

  printf ("Acquiring semaphore\n");
  while (semaphore <= 0) {
    if (process == CLIENT) {
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
    }
    else if (process == SERVER) {
      nextCommand = 'S';
      write (serverSocket[0], &nextCommand, 1);
      read (serverSocket[0], &nextCommand, 1);
    }
    else {
      fprintf (stderr, "Illegal parameter in acquire!\n");
      exit (1);
    }
  }
  semaphore --;
}

void release (int process) {
  char nextCommand;

  printf ("Releasing semaphore.\n");
  semaphore ++;
  if (process == CLIENT) {
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
    }
    else if (process == SERVER) {
      nextCommand = 'S';
      write (serverSocket[0], &nextCommand, 1);
      read (serverSocket[0], &nextCommand, 1);
    }
    else {
      fprintf (stderr, "Illegal parameter in acquire!\n");
      exit (1);
    }
}  
    

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

  char nextCommand;
  int charRead;
  int i;
  
  read (clientSocket[0], &nextCommand, 1);
  while (1) {
    acquire (CLIENT);
    while (count == BUFFERSIZE) {
      release (CLIENT);
      nextCommand = 'S';
      write (clientSocket[0], &nextCommand, 1);
      read (clientSocket[0], &nextCommand, 1);
      acquire (CLIENT);
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
      count = (count +charRead) % BUFFERSIZE;
    }
    else {
      fprintf (stderr, "Client: Too many characters read, discarding!\n");
    }
    release (CLIENT);
  }
}
  
void *server () {

  int tmp;
  char nextCommand;

  read (serverSocket[0], &nextCommand, 1);
  printf ("Server: Now executing server, count = %d\n", count);
  while (1) {
    acquire (SERVER);
    while (count == 0) {
      /* now call the scheduler */
      release (SERVER);
      nextCommand = 'S';
      write (serverSocket[0], &nextCommand, 1);
      read (serverSocket[0], &nextCommand, 1);
      acquire (SERVER);
    }
    
    
    /* adjust the index */
    currentIndex = (currentIndex - 1) %BUFFERSIZE;
    count --;

    tmp = buffer[currentIndex];
    printf ("Server: The next element in the buffer is %d\n", tmp);
    release (SERVER);
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

