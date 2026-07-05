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
const char* strrstr(const char* haystack, const char* needle);

/**
 * @return Returns true if end is at the end of string.
 */
bool strendswith(const char* str, const char* end);

bool strmatch(const char* str, const char* pattern);

const char* strsub(const char* str, size_t i, size_t j);

const char* strcopy(const char* str);

const char* strreplace(
  const char* str, const char* replacement, size_t i, size_t j);

const char* strreplacestr(
  const char* str, const char* pattern, const char* replacement);

/**
 * @brief Replaces any ascii lowercase letters with their uppercase varients.
 * Modifies the input string.
 *
 * @param  str
 * @return Returns the string pointer given.
 */
const char* strupper(char* str);

/**
 * @brief Replaces any ascii uppercase letters with their lowercase varients.
 * Modifies the input string.
 *
 * @param  str
 * @return Returns the string pointer given.
 */
const char* strlower(char* str);

const char* strrand(int len);

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

typedef struct strvec_header {
  uint32_t size;         /* Buffer size */
  const char** sentinel; /* Array sentinel pointer */
  char* strings;         /* String section start */
} strvec_header;

#define _svheader(vec)                                        \
  (vec ? (strvec_header*)((char*)vec - sizeof(strvec_header)) \
       : (strvec_header*)NULL)

const char** _svnalloc(const char** vec, int align, int bytes);

void _svappend(const char** vec, const char* str);

#define scl_svappend(vec, str, align)                   \
  {                                                     \
    (vec) = _svnalloc((vec), (align), strlen(str) + 1); \
    _svappend((vec), (str));                            \
  }

size_t _svlen(const char** vec);
#define scl_svlen(vec) _svlen(vec)

#define scl_svback(vec) \
  ((vec) ? ((const char**)(vec))[scl_svlen(vec) - 1] : NULL)

#define scl_svstrings(vec) ((vec) ? _svheader(vec)->strings : NULL)

void scl_svseparator(const char** vec, char oldSeparator, char separator);

#define scl_svfree(vec) \
  if(vec)               \
  free(_svheader(vec))
#ifdef __cplusplus
}
#endif
#endif
