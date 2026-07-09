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


/* sclc_string.h
 * extends the existing string.h helpers
 */

#ifndef sclc_string_h
#define sclc_string_h

#ifdef __cplusplus
extern "C" {
#endif

#include "sclc_base.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Finds the last instance of needle in haystack.
 *
 * @param  haystack
 * @param  needle
 * @return Returns a pointer to the last instance of needle in haystack, the
 * pointer will be in haystack, or null if no instance was found.
 */
extern SCLAPI const char* strrstr(const char* haystack, const char* needle);

/**
 * @return Returns true if end is at the end of string.
 */
extern SCLAPI bool strendswith(const char* str, const char* end);

extern SCLAPI bool strmatch(const char* str, const char* pattern);

extern SCLAPI const char* strsub(const char* str, size_t i, size_t j);

extern SCLAPI const char* strcopy(const char* str);

extern SCLAPI const char* strreplace(
  const char* str, const char* replacement, size_t i, size_t j);

extern SCLAPI const char* strreplacestr(
  const char* str, const char* pattern, const char* replacement);

/**
 * @brief Replaces any ascii lowercase letters with their uppercase varients.
 * Modifies the input string.
 *
 * @param  str
 * @return Returns the string pointer given.
 */
extern SCLAPI const char* strupper(char* str);

/**
 * @brief Replaces any ascii uppercase letters with their lowercase varients.
 * Modifies the input string.
 *
 * @param  str
 * @return Returns the string pointer given.
 */
extern SCLAPI const char* strlower(char* str);

extern SCLAPI const char* strrand(int len);

#define scl_stkfmt(buf, mx, fmt, ...)                                \
  {                                                                  \
    int __l = min(snprintf(NULL, 0, fmt, __VA_ARGS__), (int)mx - 1); \
    if(!(buf))                                                       \
      (buf) = alloca(__l + 1);                                       \
    snprintf((void*)buf, __l + 1, fmt, __VA_ARGS__);                 \
    ((char*)(buf))[__l] = 0;                                         \
  }

#define scl_stkfmtx(buf, mx, fmt, ...)                               \
  {                                                                  \
    int __l = min(snprintf(NULL, 0, fmt, __VA_ARGS__), (int)mx - 1); \
    snprintf((void*)buf, __l + 1, fmt, __VA_ARGS__);                 \
    ((char*)(buf))[__l] = 0;                                         \
  }

typedef struct strvec_header_t {
  uint32_t size;         /* Buffer size */
  const char** sentinel; /* Array sentinel pointer */
  char* strings;         /* String section start */
} strvec_header_t;

#define _svheader(vec)                                            \
  (vec ? (strvec_header_t*)((char*)vec - sizeof(strvec_header_t)) \
       : (strvec_header_t*)NULL)

extern SCLAPI const char** _svnalloc(const char** vec, int align, int bytes);

extern SCLAPI void _svappend(const char** vec, const char* str);

#define scl_svappend(vec, str, align)                   \
  {                                                     \
    (vec) = _svnalloc((vec), (align), strlen(str) + 1); \
    _svappend((vec), (str));                            \
  }

extern SCLAPI size_t _svlen(const char** vec);
#define scl_svlen(vec) _svlen(vec)

#define scl_svback(vec) \
  ((vec) ? ((const char**)(vec))[scl_svlen(vec) - 1] : NULL)

#define scl_svstrings(vec) ((vec) ? _svheader(vec)->strings : NULL)

extern SCLAPI void scl_svseparator(
  const char** vec, char oldSeparator, char separator);

#define scl_svfree(vec) \
  if(vec)               \
  free(_svheader(vec))
#ifdef __cplusplus
}
#endif
#endif
