#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#define TRUE 1
#define FALSE 0

int counter = 0;

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

void timer_handler(int signum) {
  printf("Handling signal %s (counter -> %d)\n", strsignal(signum), counter++);
  
  // Note that, although when setting up a handler we can set a mask of signals
  // it should block, initially it will always block the signal that caused it.
  // So if a SIGALRM fired again whilst we are running this handler code, it
  // will be blocked until our handler returns.

  // Sometimes, particularly when we are running a long piece of handler code,
  // we may wish to allow our handler to be interrupted by the same SIGNAL, so
  // we can explicitly unblock a signal as follows.
  sigset_t sigset;      // To hold a set of signals to be unblocked.
  sigemptyset(&sigset); // Start with an empty set of signals.
  sigaddset(&sigset, SIGALRM); // Add the SIGALRM signal to the set.
  // Now unblock the signals in the set (i.e. just SIGALRM).
  check(sigprocmask(SIG_UNBLOCK, &sigset, NULL), "Unable to unblock SIGALRM");

  // So any code here, despite being within a SIGALRM handler, can be
  // interrupted by another SIGALRM if one should occur before we return.
  
  // You can block signals in a similar way, so check out the man page of
  // sigprocmask
}

void setup_timer() {
  printf("Setting up timer handler and timer.\n");

  struct sigaction action;          // Allocate struct on stack.
  action.sa_handler = timer_handler; // Set the function pointer our handler.
  action.sa_flags = 0;              // Use no special flags.
  sigfillset(&action.sa_mask);      // Again, full set of signals blocked while this
                                    // handler runs

  // Register to handle SIGALRM signals.
  check(sigaction(SIGALRM, &action, NULL), "Failed to setup signal handler");

  // We define the behaviour of the timer with itimerval.
  struct itimerval timer_value;
  // it_value is the time from now until the signal fires.
  timer_value.it_value.tv_sec = 1;   // Seconds
  timer_value.it_value.tv_usec = 0;  // Microseconds
  // it_interval is the time between repeated signals, so here I set it
  // to the same as it_value.
  timer_value.it_interval = timer_value.it_value;
  
  // Now start the timer.  ITIMER_REAL means base the timer on real ellapsed
  // time, though it is also possible to base it on process time (e.g. the time
  // that our process has been running on the CPU)
  check(setitimer(ITIMER_REAL, &timer_value, NULL), "Failed to set timer running");
}

int main() {
  printf("Playing with signals and signal handlers\n");

  setup_timer(); 

  while(TRUE); // Spin, so process does not exit.
}
