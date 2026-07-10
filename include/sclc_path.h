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


/* sclc_path.h
 *
 */

#ifndef sclc_path_h
#define sclc_path_h

#ifdef __cplusplus
extern "C" {
#endif

#include "sclc_base.h"
#include <stdio.h>

#ifdef _WIN32
#  define PATH_MAX 260
#  define _SLASH   "\\"
#else
#  include <limits.h>
#  define _SLASH "/"
#endif

#define SCL_PATH_FILES  0
#define SCL_PATH_DIRS   1

#define scl_iswild(str) (strstr(str, "*") != NULL)

extern SCLAPI int scl_pathjoinx(char* buf, const char* one, const char* two);

#define scl_pathjoin(buf, one, two)                          \
  {                                                          \
    if(!(buf))                                               \
      (buf) = alloca(scl_pathjoinx(NULL, (one), (two)) + 1); \
    scl_pathjoinx((char*)(buf), (one), (two));               \
  }

extern SCLAPI const char* scl_realpath(const char* path, char* resolved);

extern SCLAPI const char* scl_execdir();

extern SCLAPI bool scl_pathexists(const char* path);

extern SCLAPI bool scl_isdirectory(const char* path);

extern SCLAPI bool scl_isfile(const char* path);

extern SCLAPI const char* scl_filename(const char* path);

extern SCLAPI const char* scl_pathstem(const char* path);

extern SCLAPI const char* scl_parentpath(const char* path);

extern SCLAPI const char* scl_pathcomponent(const char** path);

extern SCLAPI bool scl_chdir(const char* path);

extern SCLAPI bool scl_mkdir(const char* path);

extern SCLAPI bool scl_mkdirs(const char** paths, int count);

extern SCLAPI const char** scl_glob(
  const char* pattern, const char** finds, int mode);

extern SCLAPI int64_t scl_wtime(const char* path);

#ifdef __cplusplus
}
#endif
#endif
