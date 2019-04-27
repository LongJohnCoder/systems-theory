#include <stdio.h>
#include <stdlib.h>

// It is often very useful to create composite types, that
// basically group together several other types.  struct allows
// us to do this, and it almost (bar also containing functions)
// reminds us of an OOP class, which also group attributes.
// Here we use a struct to describe a point (i.e. coordinate).
struct Point {
  int x;
  int y;
}; // <-- Note the semi-colon, since this is a variable defintion.

// Compute the area of the rectangle that bounds two points.
int calculate_area(struct Point pt1, struct Point pt2) {
  // abs() simply inverts the sign of a negative number.
  return abs(pt1.x - pt2.x) * abs(pt1.y - pt2.y);
}

// Expands the rectangle, but args are copied and so
// actually there is no affect on original parameters.
// Recall from the earlier lecture on the use of pointers
// to enable pass-by-reference parameters.
void expand_bad(struct Point pt1, struct Point pt2) {
  // Note the use of '.' to access elements of the struct
  // much like accessing Java object attributes.
  pt1.x--;
  pt1.y--;
  pt2.x++;
  pt2.y++;
}

// Expands the rectangle, accepting pointers to the structure
// so the original parameters are affected as expected.
void expand_good(struct Point *p_pt1, struct Point *p_pt2) {
  // Now that we are working with pointers (to our Point struct),
  // it is not possible to simply write 'p_pt1.x = ...', since a pointer
  // has no attributes - it refers only to an address, that in our case contains
  // the data of our Point struct.  Since it is common to access attributes of
  // struct pointers the compiler allows us to use the arrow notation '->'.
  p_pt1->x--;
  p_pt1->y--;
  p_pt2->x++;
  p_pt2->y++;
  // For interest, the long-handed way would be to write '(*p_pt1).x--', which
  // means: dereference the thing pointed to (which yields our struct), then
  // access attribute 'x'.
}

int main () {

  // Define two points.
  struct Point point1, point2;

  // Set the first point's coords
  point1.x = 0;
  point1.y = 0;
  // Set the second point's coords
  point2.x = 2;
  point2.y = 3;

  int area = calculate_area(point1, point2);
  printf ("The original area is %d\n", area);

  expand_bad(point1, point2);
  area = calculate_area(point1, point2);
  printf ("The area after expand_bad() is %d\n", area);

  expand_good(&point1, &point2);
  area = calculate_area(point1, point2);
  printf ("The area after expand_good() is %d\n", area);
  
  return 0;
}
