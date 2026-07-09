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


/* scl_time.hpp
 * scl time helpers
 */

#ifndef scl_time_hpp
#define scl_time_hpp

#include <functional>
#include <stdarg.h>

namespace scl {

/**
 * @brief Resets the output of scl::clock(), making current time epoch.
 *
 */
void resetclock();

/**
 * @note You can use scl::resetclock() to control this function's epoch.
 *
 * @return   Seconds since epoch.
 */
double clock();

/**
 * @brief Makes this thread sleep for a given amount of milliseconds.
 *
 * @param sleemms  Number of milliseconds to sleep for.
 */
void waitms(double ms);

/**
 * @brief  Waits until the given lamda function returns true.
 *
 * @param  cond  Lambda function to be called.
 * @param  timeout  Max number of seconds to wait. By default -1 (infinite).
 * @param  sleepms  Number of milliseconds to sleep for inbetween condition
 * checks. By default 0.001ms.
 * @return  true: Wait did not time out, false: Wait did time out.
 */
bool waitUntil(
  std::function<bool()> cond, double timeout = -1, double sleepms = 0.001);
} // namespace scl

#endif
