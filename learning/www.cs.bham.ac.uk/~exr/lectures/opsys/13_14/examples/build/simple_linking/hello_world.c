#include <stdio.h>

/* In the line below, we are telling gcc to include the prototypes listed in
 * our *local* header file, hellofunctions.h.
 *
 * Note that we don't need to put hellofunctions.h in the gcc command line parameters:
 * this line makes gcc look for the file locally automatically.
 */
//#include "hellofunctions.h" 

//it's fine to do things this way in our simple example: but what if we used the function 
//in several other places, and its definition changed? What a headache!
void nowLetsPrintNTimes(char*,char**,int);

int main(){
    printf("Hello, World!\n");
    //currently hello_world doesn't know where to find nowLetsPrintNTimes - it just knows
    //that I've called the function correctly according to the prototype.
    char *names[2];

    //fill our array with names of people we want to say hello to. 
    names[0]="Matt";
    names[1]="Barry";
    nowLetsPrintNTimes("Hello",names,2);
    return 0;
}
