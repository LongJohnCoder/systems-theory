/* Based on code at http://en.wikipedia.org/wiki/Fork_(28operating_system) */
#include <stdio.h>   /* printf, stderr, fprintf */
#include <sys/types.h> /* pid_t */
#include <unistd.h>  /* _exit, fork */
#include <stdlib.h>  /* exit */
#include <errno.h>   /* errno */
#include <sys/wait.h> /* Wait for Process termination */
 
int main(void)
{
   pid_t  pid;
   int i = 0;
 
   /* Output from both the child and the parent process
    * will be written to the standard output,
    * as they both run at the same time.
    */
   pid = fork();
   if (pid == -1)
   {   
      /* Error:
       * When fork() returns -1, an error happened
       * (for example, number of processes reached the limit).
       */
      fprintf(stderr, "can't fork, error %d\n", errno);
      exit(EXIT_FAILURE);
   }
 
   if (pid == 0)
   {
      /* Child process:
       * When fork() returns 0, we are in
       * the child process.
       */
       while (i < 10) {
         printf("child: %d\n", i);
	 i++;
       }
       exit(0);  
   }
   else
   { 
 
      /* When fork() returns a positive number, we are in the parent process
       * (the fork return value is the PID of the newly created child process)
       * Again we count up to ten.
       */
       int status; /* return status of child */
      while (i < 10) {
         printf("parent: %d\n", i);
	 i++;
      }
      wait (&status); /* wait for child to exit, and store its status */
      printf("PARENT: Child's exit code is: %d\n", WEXITSTATUS(status));
      exit(0);
   }
   return 0;
}

