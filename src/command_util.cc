#include "command_util.h"
#include <tuple>
#include <algorithm>

// Point
Point Point::operator+ (const Point& p) const {
  return Point(x + p.x, y + p.y, z + p.z);
}

Point& Point::operator+= (const Point& p) {
  this->x += p.x;
  this->y += p.y;
  this->z += p.z;
  return *this;
}

Point Point::operator- (const Point& p) const {
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

// Utilities
bool IsLCD(const Point& p) {
  if ((p.x != 0 && p.y == 0 && p.z == 0) ||
      (p.x == 0 && p.y != 0 && p.z == 0) ||
      (p.x == 0 && p.y == 0 && p.z != 0)) {
    return true;
  }
  return false;
}

uint32_t MLen(const Point& p) {
  return abs(p.x) + abs(p.y) + abs(p.z);
}

uint32_t CLen(const Point& p) {
  return std::max({abs(p.x), abs(p.y), abs(p.z)});
}

bool IsSLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 5;
}

bool IsLLD(const Point& p) {
  return IsLCD(p) && MLen(p) <= 15;
}

bool IsNCD(const Point& p) {
  return 0 < MLen(p) && MLen(p) <= 2 && CLen(p) == 1;
}

bool IsFCD(const Point& p) {
  return 0 < CLen(p) && CLen(p) <= 30;
}

bool IsPath(const Point& p1, const Point& p2) {
  Point p(abs(p1.x - p2.x),
          abs(p1.y - p2.y),
          abs(p1.z - p2.z));
  return IsLCD(p);
}

bool ColidX(int from_x1, int to_x1, int from_x2, int to_x2) {

  if (from_x1 > to_x1)
    std::swap(from_x1, to_x1);

  if (from_x2 > to_x2)
    std::swap(from_x2, to_x2);

  if (from_x1 <= from_x2 && from_x2 <= to_x1)
    return true;

  if (from_x2 <= from_x1 && from_x1 <= to_x2)
    return true;

  return false;
}
