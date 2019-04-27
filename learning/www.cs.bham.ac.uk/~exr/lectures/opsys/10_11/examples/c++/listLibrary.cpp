#include <iostream>
#include <list> /* for the list-library */
#include <iterator> /* for the iterators */
using namespace std;

int main () {

  /* create variables */
  list<int> mylist;
  list<string> stringList;

  /* define iterators: used to access elements of a list */
  list<int>::const_iterator currentList;
  list<string>::const_iterator currentStringList;

  /* create empty list */
  mylist = list<int>();
  stringList = list<string>();

  /* add three elements */
  mylist.push_front (4);
  mylist.push_front (5);
  mylist.push_back (6);

  stringList.push_front ("AAA");
  stringList.push_front ("BBB");


  /* display them one by one */
  /* Iterator used like pointers */
  for (currentList = mylist.begin(); currentList != mylist.end(); ++currentList) {
    cout << "The next element is " << *currentList << "\n";

  }  
  for (currentStringList = stringList.begin(); currentStringList != stringList.end(); ++currentStringList) {
    cout << "The next element is " << *currentStringList << "\n";

  }  
  return 0;
}
 
