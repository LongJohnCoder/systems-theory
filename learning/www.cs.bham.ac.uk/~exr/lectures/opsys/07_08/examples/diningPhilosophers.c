#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>

#define NO_OF_PHILOSOPHERS 5
struct Semaphore chopstick[NO_OF_PHILOSOPHERS]; /* the chopsticks */
int communicationSocket[5][2]; /* for modelling the scheduler */

void *philosopher (void *arg) {
  char nextCommand;
  int i = *((int *) arg);

  printf ("Started philosopher %d\n", i);
  /* now wait to be scheduled */
    nextCommand = 'S';
    read (communicationSocket[i][0], &nextCommand, 1);
  while (1) {
    /* get left chopstick */
    Acquire (&(chopstick[i]), i);
    printf ("Philoshoper No. %d acquired left chopstick\n", i);

    /* now call the scheduler */
    nextCommand = 'S';
    write (communicationSocket[i][0], &nextCommand, 1);
    read (communicationSocket[i][0], &nextCommand, 1);

    /* get right chopstick */
    Acquire (&(chopstick [(i+1)%NO_OF_PHILOSOPHERS]), i);
    printf ("Philoshoper No. %d now eating \n", i);

    /* now call the scheduler - eating  */
    nextCommand = 'S';
    write (communicationSocket[i][0], &nextCommand, 1);
    read (communicationSocket[i][0], &nextCommand, 1);
    
    /* return left chopstick */
    Release (&(chopstick[i]), i);
    printf ("Philoshoper No. %d released left chopstick\n", i);
    nextCommand = 'S';
    write (communicationSocket[i][0], &nextCommand, 1);
    read (communicationSocket[i][0], &nextCommand, 1);

    /* return right chopstick */
    Release (&(chopstick[(i+1)%NO_OF_PHILOSOPHERS]), i);
    printf ("Philoshoper No. %d released right chopstick\n", i);
    nextCommand = 'S';
    write (communicationSocket[i][0], &nextCommand, 1);
    read (communicationSocket[i][0], &nextCommand, 1);
  }
}
    
int main (int argc, char **argv) {

    
  pthread_t philosophers[NO_OF_PHILOSOPHERS];
  int i;
  int *arg;
  int nextProcess;
  char nextCommand;
  
  /* initialise the semaphores */
  for (i = NO_OF_PHILOSOPHERS; i >= 0 ; i--) {
    SemaphoreInitialise (&chopstick[i]);
  }


  for (i = NO_OF_PHILOSOPHERS-1; i >= 0 ; i--) {
    /* create the sockets */
    if ((socketpair (AF_UNIX, SOCK_STREAM, 0, communicationSocket[i]) < 0 ) || socketpair (AF_UNIX, SOCK_STREAM, 0, communicationSocket[i])< 0 ) {
      fprintf (stderr, "Socket creation failed!\n");
      exit (1);
    }
  }

    /* Now create philosopher process */

  for (i = NO_OF_PHILOSOPHERS-1; i >= 0 ; i--) {
    arg = malloc (sizeof (int));
    *arg = i;
    if (pthread_create (&(philosophers[i]), NULL, philosopher, (void *) arg) == -1) { 
      fprintf (stderr, "Couldn't create philosopher %d\n", i);
      exit (1);
    }
  }
      
    /* Scheduler */
      
    while (1) {
      printf ("Scheduler: Next process to run: ");

      scanf ("%d", &nextProcess);
      printf ("\nRead number %d\n", nextProcess);
      if ((nextProcess >=0) && (nextProcess < NO_OF_PHILOSOPHERS)) {
	nextCommand = 'R';
	write (communicationSocket[nextProcess][1], &nextCommand, 1);
	read (communicationSocket[nextProcess][1], &nextCommand, 1);
      }
      else {
	fprintf (stderr, "Illegal argument read: %d\n", nextProcess);
      }
    }
    exit (0);
}
    
