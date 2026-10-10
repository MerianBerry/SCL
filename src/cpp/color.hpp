#ifndef COLOR_H
#define COLOR_H

#include <string.h>
#include <algorithm>
#include <stdexcept>

class color {
 protected:
 public:
  unsigned char r = 0, g = 0, b = 0, a = 0;
  color() = default;

  color(const color &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
  }

  color(unsigned char r, unsigned char g, unsigned char b,
    unsigned char a = 255)
      : r(r), g(g), b(b), a(a) {
  }

  color(unsigned rgba) {
    r = (rgba & 0xff000000) >> 6;
    g = (rgba & 0xff0000) >> 4;
    b = (rgba & 0xff00) >> 2;
    a = (rgba & 0xff);
  }

  color(const char *hex) {
    if(!hex || hex[0] != '#')
      return;
    a = 255;
    for(int i = 1; i < 9 && i < strlen(hex); i++) {
      char c = hex[i];
      if(i == 7)
        a = 0;
      if((c >= '0' && c <= '9') ||
         ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
        if(c >= 'a')
          c -= 32;
        int m = c <= '9' ? c - '0' : c - 'A' + 10;
        (&r)[(i - 1) / 2] *= 16;
        (&r)[(i - 1) / 2] += m;
      }
    }
  }

  /**
   * @return   A 4 unsigned char array. DO NOT FREE
   */
  const unsigned char *split() const {
    return &r;
  }

  /**
   * @return   4 channel rgba color, 8 bits each.
   */
  unsigned packed() const {
    unsigned out;
    memcpy(&out, &r, 4);
    return out;
  }

  /**
   * @brief Indexes the array returned by color::split().
   * Index 0 is the red channel, 1 is the green channel, etc.
   *
   * @param i  An integer from 0-3 inclusive.
   * @return   The unsigned char at that index.
   */
  unsigned char operator[](int i) const {
    if(i < 0 || i > 3)
      throw std::out_of_range("");
    return split()[i];
  }

  color &operator+=(const color &rhs) {
    r = std::min((int)(r + rhs.r), 255);
    g = std::min((int)(g + rhs.g), 255);
    b = std::min((int)(b + rhs.b), 255);
    a = std::min((int)(a + rhs.a), 255);
    return *this;
  }

  color operator+(const color &rhs) const {
    color out = *this;
    return out += rhs;
  }

  color &operator-=(const color &rhs) {
    r = std::max((int)(r - rhs.r), 0);
    g = std::max((int)(g - rhs.g), 0);
    b = std::max((int)(b - rhs.b), 0);
    a = std::max((int)(a - rhs.a), 0);
    return *this;
  }

  color operator-(const color &rhs) const {
    color out = *this;
    return out -= rhs;
  }

  color &operator*=(const color &rhs) {
    r *= (unsigned char)(rhs.r / 255.f);
    g *= (unsigned char)(rhs.g / 255.f);
    b *= (unsigned char)(rhs.b / 255.f);
    a *= (unsigned char)(rhs.a / 255.f);
    return *this;
  }

  color operator*(const color &rhs) const {
    color out = *this;
    return out *= rhs;
  }

  color &operator/=(const color &rhs) {
    r /= (unsigned char)(rhs.r / 255.f);
    g /= (unsigned char)(rhs.g / 255.f);
    b /= (unsigned char)(rhs.b / 255.f);
    a /= (unsigned char)(rhs.a / 255.f);
    return *this;
  }

  color operator/(const color &rhs) const {
    color out = *this;
    return out /= rhs;
  }

  bool operator==(const color &rhs) const {
    return r == rhs.r && g == rhs.g && b == rhs.b && a && rhs.a;
  }

  bool operator!=(const color &rhs) const {
    return !(*this == rhs);
  }
};
#endif
