#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define BUFFERLENGTH 256

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFERLENGTH];
    if (argc < 3) {
       fprintf (stderr, "usage %s hostname port\n", argv[0]);
       exit(1);
    }

    /* create socket */
    portno = atoi (argv[2]);
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error ("ERROR opening socket");

    /* enter connection data */
    server = gethostbyname (argv[1]);
    if (server == NULL) {
        fprintf (stderr, "ERROR, no such host\n");
        exit (1);
    }
    bzero ((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy ((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons (portno);

    /* connect to the server */
    if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) 
        error ("ERROR connecting");

    /* prepare message */
    printf ("Please enter the message: ");
    bzero (buffer, BUFFERLENGTH);
    fgets (buffer, BUFFERLENGTH, stdin);

    /* send message */
    n = write (sockfd, buffer, strlen(buffer));
    if (n < 0) 
         error ("ERROR writing to socket");
    bzero (buffer, BUFFERLENGTH);

    /* wait for reply */
    n = read (sockfd, buffer, BUFFERLENGTH -1);
    if (n < 0) 
         error ("ERROR reading from socket");
    printf ("%s\n",buffer);
    return 0;
}
