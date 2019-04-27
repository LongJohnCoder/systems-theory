#include <stdio.h>     
#include <stdlib.h>   
#include <arpa/inet.h>  // inet_ntop
#include <netdb.h>      // gethostbyname

/*
It is a two-step process to connect to an Internet server when we are using a
domain name, since domain names, though understood by humans, are not understood
by the IP protocol through which Internet systems communicate:
  - First we must resolve the domain name to an IP address.
  - Then we may use the IP address to correctly state the destination of our
    data.
Here we focus on the domain name look-up, using gethostbyname().
*/

int main(int argc, char**argv) {

  if (argc != 2) {
    printf("\nUSAGE: %s <hostname>\n\n", argv[0]);
    exit(1);
  }

  char* hostname = argv[1];
  printf("Looking up the IP address of '%s'\n", hostname);

  // The 'hostent' struct is used to describe domain name information returned
  // from a look-up, which can include multiple addresses for the same domain
  // name.
  struct hostent *hostinfo;

  // Resolve the hostname to an IP address, ultimately via a DNS look-up.  For
  // convenience, this will also accept addresses as strings (e.g.
  // '72.21.210.250') and convert them to proper address structures, allowing
  // users to specify either domain names or addresses in their applications.
  hostinfo = gethostbyname(hostname);

  // Check if the look-up failed. On error, gethostbyname sets the h_errno variable:
  if (hostinfo == NULL) {
      switch(h_errno){
          case HOST_NOT_FOUND:
              fprintf(stderr, "Unknown host %s.\n", hostname);
              break;
          case NO_DATA:
              fprintf(stderr, "The domain name is valid, but doesn't have an IP\n");
              break;
          default: 
              fprintf(stderr, "Unknown error obtaining IP for %s.\n", hostname);
      }
      exit(1);
  }

  // We are interested in the h_addr attribute of the returned hostinfo, since
  // this contains the first address.  Note that the OS networking services are
  // generic, so gethostbyname could be used with many other protocols other
  // than the now de facto IPv4, but here we are expecting a 4-byte IPv4 address
  // so cast it appropriately to the in_addr (IPv4 address) structure.
  // h_addr in fact refers to h_addr_list[0], and is #defined as such.
  struct in_addr *ip_address = (struct in_addr *) hostinfo->h_addr;
  char *realhostname = hostinfo -> h_name;

  // The IP address returned is simply 4 bytes, but often addresses are printed
  // out in the dot notation (e.g. 72.21.210.250), so here we use the function
  // inet_ntop to format our obtained address for display.
  char ip_address_string[256];
  // Flag AF_INET means: format the bytes in the Internet address family style. Note in the 
  // third parameter that we are pointing to the start of the char array.
  inet_ntop(AF_INET, ip_address, (char *) &ip_address_string, sizeof(ip_address_string));
  printf("'%s' -> %s (Official Hostname is %s)\n", hostname, ip_address_string, realhostname);

  return 0;
}
