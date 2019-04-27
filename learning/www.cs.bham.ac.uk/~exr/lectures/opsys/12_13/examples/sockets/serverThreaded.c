/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}


int isExecuted = 0;

int returnValue; /* not used; need something to keep compiler happy */
pthread_mutex_t mut; /* the lock */

/* the procedure called for each request */
void *processRequest (void *args) {
  int *newsockfd = (int *) args;
  char buffer[BUFFERLENGTH];
  int n;
  int tmp;
  
  n = read (*newsockfd, buffer, BUFFERLENGTH -1);
  if (n < 0) 
    error ("ERROR reading from socket");

  printf ("Here is the message: %s\n",buffer);
  pthread_mutex_lock (&mut); /* lock exclusive access to variable isExecuted */
  tmp = isExecuted;
  printf ("Waiting for confirmation: Please input an integer\n");
  scanf ("%d", &n); /* not to be done in real programs: don't go to sleep while holding a lock! Done here to demonstrate the mutual exclusion problem. */
  printf ("Read value %d\n", n);

  isExecuted = tmp +1;
  pthread_mutex_unlock (&mut); /* release the lock */
  n = sprintf (buffer, "I got you message, the  value of isExecuted is %d\n", isExecuted);
  /* send the reply back */
  n = write (*newsockfd, buffer, BUFFERLENGTH);
  if (n < 0) 
    error ("ERROR writing to socket");
       
  close (*newsockfd); /* important to avoid memory leak */  
  free (newsockfd);

  returnValue = 0;  /* cannot guarantee that it stays constant */
  pthread_exit (&returnValue);
}



int main(int argc, char *argv[])
{
     size_t clilen;
     int sockfd, portno;
     char buffer[BUFFERLENGTH];
     struct sockaddr_in serv_addr, cli_addr;
     pthread_t *server_thread;
     int result;



     if (argc < 2) {
         fprintf (stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     /* create socket */
     sockfd = socket (AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero ((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons (portno);

     /* bind it */
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     /* ready to accept connections */
     listen (sockfd,5);
     clilen = sizeof (cli_addr);
     
     /* now wait in an endless loop for connections and process them */
     while (1) {
       
       int *newsockfd; /* allocate memory for each instance to avoid race condition */
       pthread_attr_t pthread_attr; /* attributes for newly created thread */

       newsockfd  = malloc (sizeof (int));
       if (!newsockfd) {
	 fprintf (stderr, "Memory allocation failed!\n");
	 exit (1);
       }

       /* waiting for connections */
       *newsockfd = accept( sockfd, 
			  (struct sockaddr *) &cli_addr, 
			  &clilen);
       if (*newsockfd < 0) 
	 error ("ERROR on accept");
       bzero (buffer, BUFFERLENGTH);

     /* create separate thread for processing */
     server_thread = malloc (sizeof (pthread_t));
     if (!server_thread) {
	 fprintf (stderr, "Couldn't allocate memory for thread!\n");
	 exit (1);
       }

     if (pthread_attr_init (&pthread_attr)) {
	 fprintf (stderr, "Creating initial thread attributes failed!\n");
	 exit (1);
     }

     if (pthread_attr_setdetachstate (&pthread_attr, !PTHREAD_CREATE_DETACHED)) {
       	 fprintf (stderr, "setting thread attributes failed!\n");
	 exit (1);
     }
     result = pthread_create (server_thread, &pthread_attr, processRequest, (void *) newsockfd);
       if (result != 0) {
	 fprintf (stderr, "Thread creation failed!\n");
	 exit (1);
       }

       
     }
     return 0; 
}
