#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main () {
    pid_t pid;
    
    pid = vfork();
    if (pid == -1)    {   
      /* Error:
       * When fork() returns -1, an error happened
       * (for example, number of processes reached the limit).
       */
      fprintf(stderr, "can't fork, error %d\n", errno);
      exit(EXIT_FAILURE);
   }
    
    if (pid == 0) {
	/* child process - just exec */
	execv ("/home/exr/teaching/lectures/opsys/12_13/examples/basics/hello_world", NULL);
	fprintf(stderr, "can't execute, error %d\n", errno);
	exit(1);
    }
    else {
	/* parent process - just exit */
	exit (0);
    }
}
