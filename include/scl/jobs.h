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

/* jobs.h
 * libscl jobs/events system
 */

#ifndef SCL_JOBS_H
#define SCL_JOBS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"
#include <stdatomic.h>

/*
 * init
 * turn (sync)
 * start (async)
 * stop
 * flush (work + waitidle)
 * isworking
 * async
 */

typedef enum {
  SCL_EV_NONE = 0,
  SCL_EV_SYNC = 1,
} scl_ev_initflags;

typedef struct {
  void* data;
  atomic_bool done;
} scl_promise_t;

SCLLIB int scl_ev_init(int flags);

SCLLIB int scl_ev_async();
#ifdef __cplusplus
}
#endif
#endif
