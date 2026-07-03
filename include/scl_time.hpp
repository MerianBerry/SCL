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
