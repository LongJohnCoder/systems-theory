#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

int main (int argc, char **argv) {
    
    char *Filename = "/proc/iptraffic";
    char Buf[65537];
    char Action = '?';
    int Continue = 1;
    int Ret;
    int File;
    
    srand(time(NULL));
   
    if (argc >= 2)
    {
	if((0==strcmp(argv[1],"W"))&&(argc>=3))
	{
	    Action = 'W';
	}
	else if(0==strcmp(argv[1],"R"))
	{
            Action = 'R';
	}
	else if(0==strcmp(argv[1],"C"))
	{
            Action = 'C';
	}
    }

    switch(Action)
    {
    case 'W':

	File = open(Filename, O_WRONLY);

	break;

    case 'R':
    case 'C':

	File = open(Filename, O_RDONLY);

	break;

    default:

	fprintf (stderr, "Usage: read <option(W)> <portstring>\n");
	fprintf (stderr, "    E.g.: testip W 80,110,25\n");
	fprintf (stderr, "Usage: read <option(R,C)\n");
	fprintf (stderr, "    E.g.: testip R\n");

	exit (1);
    }

    if(File < 0 )
    {
	fprintf (stderr, "Failed To Open %s\n", Filename);

	exit(1);
    }

    while(Continue)
    {
	switch(Action)
	{
	case 'W':

		// ensure zero-terminated string is passed to module

		Ret = write(File,argv[2],strlen(argv[2])+1);

		Continue = 0;

		break;

	case 'R':

		// issue large read to module

		Ret = read(File,Buf,sizeof(Buf)-1);

		Continue = 0;

		break;

	case 'C':

		// issue large read to module

		Ret = read(File,Buf,sizeof(Buf)-1);

		break;
	}

	if(Ret <= 0)
	{
	    fprintf (stderr, "Failed To Read/Write %s\n", Filename);

	    exit(1);
	}
  
	switch(Action)
	{
	case 'R':
	case 'C':

	    printf("Read Returned %d Bytes\n",Ret);

	    // zero-terminate module output, just in case . . .

	    Buf[Ret] = '\0';

	    printf("%s",Buf);
	}

        sleep(1);
    }

    close(File);

    return(0);
}

    
    

	
