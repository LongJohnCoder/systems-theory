#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0

// Global flag for controlling system running state.  Since this may
// potentially be modified asynchronously we use an integer type that
// guarantees atomic load and store.
volatile sig_atomic_t system_running = TRUE;
// The volatile keyword simply tells the compiler not to perform optimisation
// based on the impossibility of local changes of this variable, and should be
// used if it may be changed asynchronously and without a locking mechanism.
// Nice explanation here: http://en.wikipedia.org/wiki/Volatile_variable


/*
 * Error checking functions.
 */

void error(char* error_message) {
  perror(error_message);
  exit(1);
}
void check(int result, char* error_message) {
  if (result != 0) {
    error(error_message);
  }
}


/*
 * Signal handler functions.
 */

// All handlers should be of this form <void name(int)>
void exit_handler(int signum) {
  printf("Handling signal %s\n", strsignal(signum));
  // Change this variable to allow for graceful process termination.
  system_running = FALSE;
}

void setup_exit_signal_handler() {
  printf("Setting up signal handler to catch termination signals.\n");

  // Populate a sigaction structure, which describes how we wish to handler the
  // signal
  struct sigaction action;          // Allocate struct on stack.
  action.sa_handler = exit_handler; // Set the function pointer our handler.
  action.sa_flags = 0;              // Use no special flags.
  
  // The next line sets the signal mask of our handler, which means the set of
  // signals that will be blocked whilst this handler runs, since usually we do
  // not want signal handlers to be interrupted by other signals.  In this
  // particular case, we setting this to block the full set of possible signals
  // within our handler.  Note the use of ampersand here, which means pass the
  // address of the sigset_t struct which is an attribute of the sigaction
  // struct (see the man page of signal.h)
  sigfillset(&action.sa_mask);


  // Use this handler for terminal interrupts (e.g. pressing CTRL-C)
  check(sigaction(SIGINT, &action, NULL), "Failed to setup signal handler");
  // And also using this handler for the 'shutting down' signal.
  check(sigaction(SIGTERM, &action, NULL), "Failed to setup signal handler");
}



int main() {
  printf("Playing with signals and signal handlers\n");

  // Try removing this and pressing CTRL-C.
  setup_exit_signal_handler(); 

  while(system_running) {
    int i;
    printf("."); fflush(stdout); // Printf a dot and flush the output buffer.
    // Spin for a while, so we don't print too many chars.
    for (i=0; i<100000000;i++){};
  }
  //at this point, we must have received a signal
  printf("\nMaking a clean exit.\n");
  return 0;
}
