/*
 * Simple multithreading test: thread sums some numbers
 * See Silberschatz et al., p. 161
 *
 * Note: to compile this, add the flag -pthread to your gcc call.
 */

#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>

int sum; //data which the thread will share
void *threadFunction(void *param); //the thread

int main(int argc, char**argv){
    pthread_t tid; //thread ID
    pthread_attr_t attr; //thread attributes

    if(argc!=2){
        fprintf(stderr,"%s: Usage: %s <integer>\n",*argv,*argv);
        exit(1);
    }

    //convert the command-line parameter to an int. If it's not positive, exit
    int val = atoi(*(argv+1));
    if(val<0){
        fprintf(stderr,"%s: Parameter must be >=0 and you supplied %d.\n",*argv,val);
        exit(1);
    }

    //get the default attributes of pthread
    pthread_attr_init(&attr);
    //create the thread, which will cause threadFunction() to run. Note that
    //pthread_create requires a void* parameter last, which char* fits into, but
    //int does not.
    pthread_create(&tid,&attr,threadFunction,(void *) &val); // watch out for lifetime of memory - in general need to use malloc */
    //wait for the thread to exit, using join
    pthread_join(tid,NULL);
    
    //at this point the thread we created is dead! :(
    printf("Thread finished! Sum is %d\n",sum);
    return 0;
}

void *threadFunction(void *param){
    //param is val
    
    int i=0;
    int upper;
    
    upper = *((int *) param);
    //note that the thread shares sum
    sum=0;
    for(i=1;i<=upper;i++){
        sum += i;
    }
    //this thread can now die with honour!
    printf("Thread says sum is %d\n",sum);
    pthread_exit(0);
}


