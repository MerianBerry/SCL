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
#include "internal.hpp"

#ifdef _WIN32
#  ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#    define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#  include <time.h>
#  include <math.h>
#endif


namespace scl {
#ifdef _WIN32
/* Windows sleep in 100ns units */
static BOOLEAN _nanosleep(LONGLONG ns) {
  // h_loadWinAPI();
  ns /= 100;
  /* Declarations */
  HANDLE timer;     /* Timer handle */
  LARGE_INTEGER li; /* Time defintion */
  /* Create timer */
  if(!(timer = CreateWaitableTimerExW(nullptr,
         nullptr,
         CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
         TIMER_ALL_ACCESS))) {
    return FALSE;
  }
  /* Set timer properties */
  li.QuadPart = -ns;
  if(!SetWaitableTimer(timer, &li, 0, nullptr, nullptr, FALSE)) {
    CloseHandle(timer);
    return FALSE;
  }
  /* Start & wait for timer */
  WaitForSingleObject(timer, INFINITE);
  /* Clean resources */
  CloseHandle(timer);
  /* Slept without problems */
  return TRUE;
}

static LARGE_INTEGER base_clock = {{0}};
#else
static double base_clock = 0.0;
#endif

void resetclock() {
#ifdef _WIN32
  QueryPerformanceCounter(&base_clock);
#else
  base_clock = clock();
#endif
}

double clock() {
#if defined(_WIN32)
  LARGE_INTEGER pc;
  LARGE_INTEGER pf;
  QueryPerformanceCounter(&pc);
  QueryPerformanceFrequency(&pf);
  return (double)(pc.QuadPart - base_clock.QuadPart) / (double)pf.QuadPart;
#elif defined(__unix__) || defined(__APPLE__)
  struct timespec ts;
  timespec_get(&ts, 1);
  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) - base_clock;
#endif
}

void waitms(double ms) {
#if defined(__unix__) || defined(__APPLE__)
  struct timespec ts = {2000, 0};
  ts.tv_sec = ms / 1000.0;

  ts.tv_nsec = fmodf(ms, 1000) * 1000000.0;
  while(nanosleep(&ts, &ts) == -1)
    ;
#elif defined(_WIN32)
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  LARGE_INTEGER lf;
  QueryPerformanceFrequency(&lf);
  while(1) {
    LARGE_INTEGER li2;
    QueryPerformanceCounter(&li2);
    if((double)(li2.QuadPart - li.QuadPart) / (double)lf.QuadPart * 1000.0 >
      ms) {
      break;
    }
    _nanosleep(1000);
  }
#endif
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
