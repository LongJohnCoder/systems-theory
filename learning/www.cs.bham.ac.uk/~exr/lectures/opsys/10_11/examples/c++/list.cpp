using namespace std;
#include <iostream>

/* define lists with arbitrary type as parameter 
   Will use pointers to lists and represent empty list as null pointer */
template <class T> /* works for any arbitrary class T */
class List {
private:
  T  element;
  List *next;
public:
  /* constructor: cons */
 List (T head, List *tail) {
   element = head;
   next = tail;
 } ;

  /* the selection functions */
  T head() {
    return element;
  };

  List *tail(){
    return next;
  };
};


int main () {

  List <int> *intlist; /* pinter to list of integers */
  List <string> * stringlist; /* pointer to list of strings */

  /* create a two-element list of integers and a two-element list of strings */
  intlist = new List<int> (4, NULL);
  stringlist = new List <string> ("This is a string", NULL);

  intlist = new List <int> (5, intlist);
  stringlist = new List <string> ("This is another string", stringlist);


  /* print the elements of the integer list */
  while (intlist != NULL) {
    cout << "The next element of the integer list is " << intlist->head() << "\n";
    intlist = intlist->tail();
  }

  /* print the elements of the string list */
  while (stringlist != NULL) {
    cout << "The next element of the stringlist list is " << stringlist->head() << "\n";
    stringlist = stringlist->tail();
  }

  return 0;
}
