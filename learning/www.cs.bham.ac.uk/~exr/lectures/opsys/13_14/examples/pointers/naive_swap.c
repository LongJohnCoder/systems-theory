/*  In this program, we will demonstrate that C passes arguments to functions
 *  by _value_, not by reference, as standard. This means that what we expect
 *  to happen won't always actually happen.
 */
 
#include<stdio.h>

void swap(int,int);

int main(){
    int x=4;
    int y=5;
    
    //obviously this prints "x is 4, y is 5"
    printf("x is %d, y is %d\n",x,y);
    swap(x,y);

    //but how about this?
    printf("After calling swap, x is %d, y is %d",x,y);
    return (0);

}

/* In this method, we (naively) try to swap two variables x and y
 * by using the standard swapping technique which we've used for years.
 */
void swap(int x, int y){
    int temp = x;
    x = y;
    y = temp;
    printf("x is %d, y is %d inside swap\n",x,y);
}
