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


#include <sclc_string.h>
#include <stdlib.h>
#include <string.h>

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

const char *strrstr(const char *haystack, const char *needle) {
  if (!haystack || !needle)
    return NULL;
  size_t i = strlen(haystack);
  const size_t J = strlen(needle);
  if (!J)
    return NULL;
  if (!i)
    return NULL;
  size_t j = J;
  for (;; i--, j--) {
    /* Reset search if not applicable */
    if (haystack[i] != needle[j])
      /* needs plus one due to the decrement */
      j = J + 1;
    if (!i || !j)
      break;
  }
  /* if the start of needle was reached, found the last instance.
   * so return where i is in the haystack. */
  return !j ? &haystack[i] : NULL;
}

bool strendswith(const char *str, const char *end) {
  if (!str || !end)
    return false;
  size_t i = strlen(str);
  size_t j = strlen(end);
  if (!j)
    return true;
  if (!i)
    return false;
  for (;; i--, j--) {
    if (str[i] != end[j])
      return false;
    if (!i || !j)
      break;
  }
  return true;
}

static bool _strmatch(const char *pattern, const char *candidate, int32_t p,
                      int32_t c) {
  if (pattern[p] == '\0') {
    return candidate[c] == '\0';
  } else if (pattern[p] == '*') {
    for (;; c++) {
      if (candidate[c] == '\0' || candidate[c] == pattern[p + 1])
        break;
    }
    return _strmatch(pattern, candidate, p + 1, c);
  } else if (candidate[c] == pattern[p]) {
    return _strmatch(pattern, candidate, p + 1, c + 1);
  }
  return false;
}

bool strmatch(const char *str, const char *pattern) {
  if (!str || !pattern)
    return false;
  return _strmatch(pattern, str, 0, 0);
}

const char *strsub(const char *str, size_t i, size_t j) {
  if (!str)
    return NULL;
  const size_t l1 = strlen(str);
  if (i >= l1)
    return NULL;
  j = min(j, l1 - i);
  char *ptr = (char *)malloc(j + 1);
  memcpy(ptr, str + i, j);
  ptr[j] = 0;
  return ptr;
}

const char *strcopy(const char *str) {
  if (!str)
    return NULL;
  const size_t l = strlen(str);
  char *ptr = (char *)malloc(l + 1);
  memcpy(ptr, str, l + 1);
  return ptr;
}

const char *strreplace(const char *str, const char *replacement, size_t i,
                       size_t j) {
  if (!str || !replacement)
    return NULL;
  const size_t l1 = strlen(str);
  if (i >= l1)
    return NULL;
  const size_t l2 = strlen(replacement);
  j = min(j, l1 - i);
  /* l1 - i - j will never be less than 0, so its ok */
  const size_t d = l1 - i - j;
  char *ptr = (char *)malloc(i + l2 + d + 1);
  // memset(ptr, 0, i + l2 + d + 1);
  memcpy(ptr, str, l1);
  /* if necessary, move post replacement text back */
  if (d)
    memcpy(ptr + i + l2, str + i + j, d);
  memcpy(ptr + i, replacement, l2);
  ptr[i + l2 + d] = 0;
  return ptr;
}

const char *strreplacestr(const char *str, const char *pattern,
                          const char *replacement) {
  if (!str || !pattern || !replacement)
    return NULL;
  const size_t l1 = strlen(str);
  const ssize_t l2 = strlen(replacement);
  const size_t J = strlen(pattern);
  ssize_t d = 0;
  size_t i = 0;
  size_t j = 0;
  /* find the final string size, to reduce reallocations */
  for (; i < l1; i++) {
    if (str[i] != pattern[j]) {
      j = 0;
      continue;
    }
    if (j == J) {
      j = 0;
      d += l2 - (ssize_t)J;
      continue;
    }
    j++;
  }
  i = 0;
  j = 0;
  const size_t fl = l1 + d;
  char *ptr = (char *)malloc(fl + 1);
  /* copy and replace */
  for (; *str && i < fl; str++) {
    if (*str != pattern[j]) {
      j = 0;
      ptr[i] = *str;
      i++;
      continue;
    }
    if (j == J) {
      memcpy(ptr + i, replacement, l2);
      ptr += i;
      j = 0;
      continue;
    }
    j++;
  }
  ptr[fl] = 0;
  return ptr;
}

const char *strupper(char *str) {
  if (!str)
    return NULL;
  size_t l = strlen(str);
  for (size_t i = 0; i < l; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 32;
    }
  }
  return str;
}

const char *strlower(char *str) {
  if (!str)
    return NULL;
  size_t l = strlen(str);
  for (size_t i = 0; i < l; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] += 32;
    }
  }
  return str;
}

static int rand_int(int min, int max) {
  return (abs(rand()) % (max - min + 1)) + min;
}

const char *strrand(int len) {
  static const char rchars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  if (len <= 0)
    return NULL;
  char *ptr = (char *)malloc((size_t)len + 1);
  ptr[len] = 0;
  for (int i = 0; i < len; i++) {
    ptr[i] = rchars[rand_int(0, sizeof(rchars) - 1)];
  }
  return ptr;
}

#define _svalignup(x, align) ((((x) + ((align) - 1)) / (align)) * align)

const char **_svnalloc(const char **vec, int align, int bytes) {
  strvec_header_t *header = _svheader(vec);
  /* check for potential strings/sentinel crossover (undersize) */
  if (!header ||
      header->strings - bytes < (char *)header->sentinel + sizeof(void *) * 2) {
    int i;
    bytes = _svalignup((header ? header->size : 0) + bytes, align);
    void *buffer = malloc(sizeof(*header) + bytes);
    char *strings = (char *)buffer + sizeof(*header) + bytes;
    *--strings = 0x7f; /* strings sentinel */
    const char **nvec = (const char **)((char *)buffer + sizeof(*header));
    for (i = 0; vec && vec[i]; i++) {
      const char *string = vec[i];
      const uint32_t ssize = strlen(string) + 1;
      strings -= ssize;
      memcpy(strings, string, ssize);
      nvec[i] = strings;
    }
    nvec[i] = NULL; /* sentinel */
    if (header)
      free(header);
    header = buffer;
    header->size = bytes;
    header->sentinel = &nvec[i];
    header->strings = strings;
    vec = (const char **)(((char *)header) + sizeof(*header));
  }
  return vec;
}

void _svappend(const char **vec, const char *str) {
  strvec_header_t *header = _svheader(vec);
  if (!header || !str)
    return;
  const uint32_t ssize = strlen(str) + 1;
  header->strings -= ssize;
  memcpy(header->strings, str, ssize);
  *header->sentinel = header->strings;
  *++header->sentinel = NULL;
}

size_t _svlen(const char **vec) {
  strvec_header_t *header = _svheader(vec);
  if (!header)
    return 0;
  return header->sentinel - vec;
}

void scl_svseparator(const char **vec, char oldSeparator, char separator) {
  strvec_header_t *header = _svheader(vec);
  if (!header)
    return;
  char *s = header->strings;
  while (true) {
    if (*s == oldSeparator || !*s) {
      *s = separator;
      if (*++s == 0x7f) /* encountered sentinel (007f) */ {
        *(s - 1) = 0; /* restore sentinel */
        break;
      }
    }
    s++;
  }
}
