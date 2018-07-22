#ifndef COMMAND_UTIL_H
#define COMMAND_UTIL_H

#include <iostream>

struct Point {
  int x;
  int y;
  int z;

//  Point() = default;
  Point() : x(0), y(0), z(0) {}
  Point(int x, int y, int z) : x(x), y(y), z(z) {}
  Point operator+ (const Point& p) const;
  Point& operator+= (const Point& p);
  Point operator- (const Point& p) const;
  Point& operator-= (const Point& p);
  Point operator* (int a) const;
  Point& operator*= (int a);

  int Manhattan() const {
    return std::abs(x) + std::abs(y) + std::abs(z);
  }
};

bool operator==(const Point& p1, const Point& p2);
bool operator!=(const Point& p1, const Point& p2);
bool operator<(const Point& p1, const Point& p2);
bool operator>(const Point& p1, const Point& p2);
std::ostream& operator << (std::ostream &out, const Point &p);

bool IsLCD(const Point& p);
uint32_t MLen(const Point& p);
uint32_t CLen(const Point& p);
bool IsSLD(const Point& p);
bool IsLLD(const Point& p);
bool IsNCD(const Point& p);
bool IsFCD(const Point& p);
bool IsPath(const Point& p1, const Point& p2);
bool ColidX(int from_x1, int to_x1, int from_x2, int to_x2);
#endif
