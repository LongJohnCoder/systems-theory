using namespace std;
#include <iostream>

/* abstract class */
class Polygon {
 protected: 
  int width, height;
 public:
  Polygon (int, int);
  virtual int area ()  =0; /*  will *not* provide implementation, but the concrete class must implement it */
};

/* it has a constructor */
Polygon::Polygon (int w, int h) {
  width = w;
  height = h;
}

/* subclass 1 */
class Rectangle: public Polygon {
public:
  Rectangle (int, int);
  int area ();
};
  
/* its constructor */
Rectangle::Rectangle (int w, int h) : Polygon (w, h) {
  /* use the constructor of Polygon */
};

/* and its method of calculating the area */
int Rectangle::area () {
  return (width * height);
}

/* subclass 2 */
class Square: public Polygon {
public:
  Square (int);
  int area ();
};
  
/* its constructor */
Square::Square (int w) : Polygon (w, w) {
  /* use the constructor of Polygon with special arguments */
};

/* its method of calculating the area */
int Square::area () {
  return (width * width);
}


int main () {
  Rectangle *rectangle;
  Square *square;

  rectangle = new Rectangle (4, 5);
  cout << "The area of the rectangle is " << rectangle->area() << "\n";
  square = new Square (4);
  cout << "The area of the square is " << square->area() << "\n";

  return 0;
}
