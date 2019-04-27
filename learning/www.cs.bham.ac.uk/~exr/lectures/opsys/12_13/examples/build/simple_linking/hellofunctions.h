//these preprocessor arguments are good practice: given that several sourcecode files
//could be loading in this header file, why do it more than once?
#ifndef _HELLOFUNCTIONS_H
#define _HELLOFUNCTIONS_H

//in this header file, we just describe the *prototypes* for 
//the functions we're defining in other places (more_hello_world.c).

void nowLetsPrintNTimes(char *str, char **names, int size);

#endif
