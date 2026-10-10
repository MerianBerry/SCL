/*
  Copyright (c) 2026 Merian

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/* time.cpp
 * scl time helpers
 */

#include <scl_time.hpp>
#include <sclc_time.h>
#include "internal.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <time.h>
#  include <math.h>
#endif


namespace scl {
void resetclock() {
  scl_resetclock();
}

double clock() {
  return scl_clock();
}

void waitms(double ms) {
  return scl_waitms(ms);
}

bool waitUntil(std::function<bool()> cond, double timeout, double sleepms) {
  bool infinite = timeout == -1;
  double cs = scl::clock();
  double ce;
  bool timedout = false;
  while(!cond() && !timedout) {
    scl::waitms(sleepms);
    ce = scl::clock();
    timedout = ((ce - cs) > timeout && !infinite);
  }
  return !timedout;
}
} // namespace scl
