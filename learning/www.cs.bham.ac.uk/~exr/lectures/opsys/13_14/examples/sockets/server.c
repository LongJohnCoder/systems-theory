/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     socklen_t clilen;
     int sockfd, newsockfd, portno;
     char buffer[BUFFERLENGTH];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
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
       
       /* waiting for connections */
       newsockfd = accept( sockfd, 
			  (struct sockaddr *) &cli_addr, 
			  &clilen);
       if (newsockfd < 0) 
	 error ("ERROR on accept");
       bzero (buffer, BUFFERLENGTH);
       
       /* read the data */
       n = read (newsockfd, buffer, BUFFERLENGTH -1);
       if (n < 0) 
	 error ("ERROR reading from socket");
       printf ("Here is the message: %s\n",buffer);

       /* send the reply back */
       n = write (newsockfd,"I got your message",18);
       if (n < 0) 
	 error ("ERROR writing to socket");
       
       close (newsockfd); /* important to avoid memory leak */
     }
     return 0; 
}
