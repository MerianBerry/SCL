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

/* glob.h
 * libscl filesystem glob functions
 */

#ifndef SCL_GLOB_H
#define SCL_GLOB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"
#include "uv.h"

/*
 * execdir
 * exists
 * isdir
 * isfile
 * chdir
 * copy
 * mkdir
 * mkdirs
 * glob (just reimpl for now, add extra search stuff later)
 * timespec
 */

typedef struct scl_glob_s {
  const char* pattern;
  const char** finds;
  size_t count;
} scl_glob_t;

SCLLIB void scl_globfree(scl_glob_t* glob);
/**
 * @brief
 * @warning Ensure glob.finds is NULL or valid before this call, or it will
 * segfault.
 * @param  glob
 * @param  mode
 * @return
 */
SCLLIB int scl_glob(scl_glob_t* glob, int mode);

SCLLIB int scl_globAsync(
  scl_glob_t* glob, int mode, uv_work_t* work, uv_after_work_cb cb);

#ifdef __cplusplus
}
#endif
#endif
