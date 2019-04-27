#include<stdio.h>

void nowLetsPrintNTimes(char *str,char**names,int size){
    int i;

    for(i=0; i<size; i++){
        printf("%s, %s!\n",str,*(names+i));
    }
}
