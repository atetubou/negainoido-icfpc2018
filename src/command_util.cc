#include "command_util.h"
#include <tuple>

// Point
Point Point::operator+ (const Point& p) {
  return Point(x + p.x, y + p.y, z + p.z);
}

Point& Point::operator+= (const Point& p) {
  this->x += p.x;
  this->y += p.y;
  this->z += p.z;
  return *this;
}

Point Point::operator- (const Point& p) {
  return Point(x - p.x, y - p.y, z - p.z);
}

Point& Point::operator-= (const Point& p) {
  this->x -= p.x;
  this->y -= p.y;
  this->z -= p.z;
  return *this;
}

bool operator==(const Point& p1, const Point& p2) {
    return
      p1.x == p2.x &&
      p1.y == p2.y &&
      p1.z == p2.z;
}

bool operator!=(const Point& p1, const Point& p2) {
    return !(p1 == p2);
}

bool operator<(const Point& p1, const Point& p2) {
  return std::make_tuple(p1.x, p1.z, p1.y) < std::make_tuple(p2.x, p2.z, p2.y);
}

bool operator>(const Point& p1, const Point& p2) {
  return std::make_tuple(p1.x, p1.z, p1.y) > std::make_tuple(p2.x, p2.z, p2.y);
}

std::ostream& operator << (std::ostream &out, const Point &p) {
  out << "(" << p.x << ", " << p.y << ", " << p.z << ")";
  return out;
}
