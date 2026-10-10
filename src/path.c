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


#include "scl/path.h"
#include "scl/string.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef min
#  undef min
#  undef max
#endif
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <direct.h>
#  include <io.h>
#  include <windows.h>
#  define access _access
#  define F_OK   0
#  pragma comment(lib, "Shlwapi.lib")
#else
#  include <dirent.h>
#  include <limits.h>
#  include <unistd.h>
#  ifdef __APPLE__
#    include <libproc.h>
#  endif
#endif

#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif


int scl_pathjoinx(char* buf, const char* one, const char* two) {
  if(!one || !two)
    return 0;
  bool slash = one && *one;
  /* dont add separator if already present in buf */
  if(one && strlen(one) > 0)
    slash = !!strcmp(&one[strlen(one) - 1], _SLASH);
  const int slashSize = slash ? sizeof(_SLASH) - 1 : 0;
  const int l1 = min((int)strlen(one), PATH_MAX - slashSize);
  const int l2 = min((int)strlen(two), PATH_MAX - slashSize - l1);
  const size_t L = min(l2 + (l1 ? (l1 + slashSize) : 1), PATH_MAX - 1);
  if(!buf)
    return L;
  if(l1) {
    memcpy((char*)(buf), one, l1);
    if(slashSize)
      memcpy((char*)(buf) + l1, _SLASH, sizeof(_SLASH) - 1);
  }
  if(l2) {
    const int p3 = l1 + slashSize;
    memcpy((char*)(buf) + p3, two, l2);
  }
  ((char*)(buf))[L] = 0;
  return L;
}

const char* scl_filename(const char* path) {
  if(!path)
    return NULL;
  const size_t l = strlen(path);
  if(!l)
    return NULL;
  const char* p = path + strlen(path) - 1;
  for(; p >= path; p--) {
    if(*p == '\\' || *p == '/')
      return strsub(p + 1, 0, -1);
  }
  return strcopy(path);
}

const char* scl_pathext(const char* path) {
  if(!path)
    return NULL;
  const char* p = strrstr(path, ".");
  if(p)
    return strsub(p, 0, -1);
  else
    return NULL;
}

const char* scl_pathstem(const char* path) {
  if(!path)
    return NULL;
  const char* p = path + strlen(path) - 1;
  for(; p >= path; p--) {
    if(*p == '\\' || *p == '/') {
      const char* p2 = strrstr(p, ".");
      p++;
      if(p2)
        return strsub(p, 0, p2 - p);
      else
        return strsub(p, 0, -1);
    }
  }
  return NULL;
}

const char* scl_parentpath(const char* path) {
  if(!path)
    return NULL;
  size_t l = strlen(path);
  if(!l)
    return NULL;
  int i;
  for(i = l - 1; i > 0; i--) {
    if(path[i] == '\\' || path[i] == '/') {
      do {
        i--;
      } while(path[i] == '\\' || path[i] == '/');
      break;
    }
  }
  if(i <= 0)
    return NULL;
  return strncopy(path, (size_t)i + 1);
}

const char* scl_pathcomponent(const char** path) {
  if(!path || !*path)
    return NULL;
  const char *s = *path, *p = s, *e = s;
  for(; *p; p++) {
    if(*p == '\\' || *p == '/') {
      e = p;
      do {
        p++;
      } while(*p == '\\' || *p == '/');
      break;
    }
  }
  if(e == s)
    e = p;
  *path = p;
  if(e == s)
    return NULL;
  const char* component = strsub(s, 0, e - s);
  if(component && strstr(component, "**")) {
    free((void*)component);
    component = strcopy("**");
  }
  return component;
}
