using namespace std;
#include <iostream>

/* class definition */
class CRectangle {
private:
  int width, height;
public: 
  int area(void);
  CRectangle (int, int);
};

/* constructor; no destructor necessary */
CRectangle::CRectangle (int w, int h) {
  width = w;
  height = h;
}

/* area-method: calculates area of a rectangle */
int CRectangle::area() {
  return (width * height);
}

int main () {
  CRectangle *rectangle; /* define pointer to object */

  rectangle = new CRectangle(4, 5); /* create object and assign pointer to it */
  cout << "The area is " << rectangle->area() << "\n"; /* use method to calculate area */ 
  return 0;
}


