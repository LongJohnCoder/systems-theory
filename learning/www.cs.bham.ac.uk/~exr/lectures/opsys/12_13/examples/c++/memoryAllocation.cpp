#include <iostream>
#include <list>
#include <iterator>
using namespace std;

  class Strings {
  public: 
    string s;

    /* Constructor function */
    Strings () {
      cout << "String object initialised\n";
    }

    /* Destructor function */
    ~Strings () {
      cout << "String object destroyed \n";
    }
  };


int main (int argc, char **argv) {


  int i = 1;
  
  list<Strings> *myList; /* pointer to class */
  Strings currentArgument; /* global class variable */
  list<Strings>::iterator listitem, next, tmp;

  myList = new list<Strings>(); /* create new list, initially empty */
  while (i < argc) {
    currentArgument.s = argv[i];
    i++;
    cout << "The next argument is " << currentArgument.s <<"\n";
    myList->push_front (currentArgument); /* a copy of currentArgument is created and stored in the list */
  }

  /* display constructed list */
  listitem = myList->begin();
  while (listitem != myList->end()) {
    cout << "The following argument is " << listitem->s << "\n";
    listitem++;
  }

  listitem = myList->begin();
  listitem++;
  delete (myList); 
  /* this produces random output, including segmentation fault - listitem is no longer valid! */ 
  cout << "Trying to access a deleted list" << listitem->s << "\n";


  /* Just print the argument list - as a debugging tool */
  i = 1;
  while (i < argc) {
    cout << "The next argument is " << argv[i] <<"\n";
    i++;
  }
  return 0;

}
