/*  In this program, we create some variables and demonstrate
 *  the interaction between them caused by manipulating a pointer.
 *  Try altering what the pointer points to, to see how things change.
 */
#include<stdio.h>

int main(void){
    int x=1, y=2, z[10];
    int *ip; //ip is a pointer which points to an integer
    ip = &x; //ip now points to x's location in memory
    
    y = *ip; //y is set to be whatever value ip points to (1)
    *ip = 0; //x is now set to 0. What will y be?
    ip = &z[0]; //ip now points to the first element of z
    *ip = 6498;

    printf("x is %d. y is %d. z[0] is %d.\n",x,y,*ip);

    *(++ip) = 1234; //as a pointer is really just a number, it can be
                  //incremented. What will this do? (More later on this)
    printf("z[1] is %d\n",z[1]);
    return (0);
}
