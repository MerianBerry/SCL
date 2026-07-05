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

int scl_pathjoinx(char* buf, const char* one, const char* two);

#define scl_pathjoin(buf, one, two)                          \
  {                                                          \
    if(!(buf))                                               \
      (buf) = alloca(scl_pathjoinx(NULL, (one), (two)) + 1); \
    scl_pathjoinx((char*)(buf), (one), (two));               \
  }

const char* scl_pathabs(const char* path, char* resolved);

const char* scl_execdir();

bool scl_pathexists(const char* path);

bool scl_isdirectory(const char* path);

bool scl_isfile(const char* path);

const char* scl_filename(const char* path);

const char* scl_pathstem(const char* path);

const char* scl_parentpath(const char* path);

const char* scl_pathcomponent(const char** path);

bool scl_chdir(const char* path);

bool scl_mkdir(const char* path);

bool scl_mkdirs(const char** paths, int count);

const char** scl_glob(const char* pattern, const char** finds, int mode);

int64_t scl_wtime(const char* path);

#ifdef __cplusplus
}
#endif
#endif
