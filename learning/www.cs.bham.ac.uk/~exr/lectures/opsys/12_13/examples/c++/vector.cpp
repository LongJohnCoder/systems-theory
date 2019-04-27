using namespace std;
#include <iostream>
#include <vector>
#include <algorithm>

void printInt (int n) {
  cout << "The next element is " << n << "\n";
}
  

int main () {
  vector<int> V;
  

  V =  vector<int>(3); /* can do allocation also directly */
  V[0] = 4;
  V[2] = 6;
  V[1] = 5;

  for_each (V.begin(), V.end(), printInt);


  V.resize (7);
  V[3] = 2;
  V[4] = 1;
  V[5] = 3;
  V[6] = 0;
  
  cout << "After inserting\n";

  for_each (V.rbegin(), V.rend(), printInt);

  return 0;
}
