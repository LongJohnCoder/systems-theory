#include<stdio.h>
#include<stdlib.h>

/*in this simple program, we read input from the user using the standard C library's
  getchar() function. Note that getchar() returns an int, for a good reason: we must accept
  the EOF character (^D) as a valid character, and it's too big to be a char.
  
  The program will work by asking the user at the command line how many characters they wish to 
  enter, and will then take that many inputs, and print them out. 
  */

int main(int argc, char* argv[]){
   //first, we make sure the user entered a number of characters:
   if(argc!=2){
       perror("Usage: simpleinput <numberOfCharacters>\n");
       exit(1);
   }
   //we're good to begin. First, we need to get that number. Unfortunately, 
   //it's a string. We need to convert it into an integer:
   int charsToGet = atoi(argv[1]);
   //the ternary operator can be used, like in java, to control printf in-line:
   printf((charsToGet==1)?"Getting %d character:\n":"Getting %d characters:\n",charsToGet);

   int currentChar;
   int charsCollected=0;
   //break this loop down into its constituent parts. We began with setting up a
   //new char variable currentChar. In the first part of the while test,
   //we set currentChar to get the next char from standard input. 
   //Providing that isn't EOF, we check whether we've got four characters yet. If not, we 
   //increment that variable, then print both out.
   while((currentChar=getchar())!=EOF && charsCollected++<charsToGet){
        printf("\tCharacter %d: %c\n",charsCollected,currentChar);
   }
   if(currentChar==EOF){
       printf("Input ended early.\n");
       exit(1);
   }
   //note that we're getting charsToGet *characters* here, not charsToGet lines of one character 
   //each. The return key actually counts as a character. How could we change the 
   //code so that it didn't?
}
