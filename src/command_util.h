#ifndef COMMAND_UTIL_H
#define COMMAND_UTIL_H

#include <iostream>

struct Point {
  int x;
  int y;
  int z;

  Point() = default;
  Point(int x, int y, int z) : x(x), y(y), z(z) {}
  Point operator+ (const Point& p) const;
  Point& operator+= (const Point& p);
  Point operator- (const Point& p) const;
  Point& operator-= (const Point& p);
};

bool operator==(const Point& p1, const Point& p2);
bool operator!=(const Point& p1, const Point& p2);
bool operator<(const Point& p1, const Point& p2);
bool operator>(const Point& p1, const Point& p2);
std::ostream& operator << (std::ostream &out, const Point &p);
#endif
