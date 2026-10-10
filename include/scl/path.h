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


/* path.h
 *
 */

#ifndef SCL_PATH_H
#define SCL_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"
#include <stdio.h>

#ifdef _WIN32
#  define PATH_MAX 260
#else
#  include <limits.h>
#endif
#define _SLASH          "/"

#define SCL_GLOB_FILES  0
#define SCL_GLOB_DIRS   1

#define scl_iswild(str) (str ? strstr(str, "*") != NULL : 0)

SCLLIB int scl_pathjoinx(char* buf, const char* one, const char* two);

#define scl_pathjoin(buf, one, two)                          \
  do {                                                       \
    if(!(buf))                                               \
      (buf) = alloca(scl_pathjoinx(NULL, (one), (two)) + 1); \
    scl_pathjoinx((char*)(buf), (one), (two));               \
  } while(0)

SCLLIB const char* scl_filename(const char* path);

SCLLIB const char* scl_pathext(const char* path);

SCLLIB const char* scl_pathstem(const char* path);

SCLLIB const char* scl_parentpath(const char* path);

SCLLIB const char* scl_pathcomponent(const char** path);

#ifdef __cplusplus
}
#endif
#endif
