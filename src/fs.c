
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

#include "scl/fs.h"
#include "scl/string.h"
#include "scl/path.h"
#include "uv.h"

#include <stdlib.h>
#include <string.h>

#ifndef S_IFMT
#  define S_IFMT  0xF000
#  define S_IFREG 0x8000
#  define S_IFDIR 0x4000
#endif
#ifndef S_ISREG
#  define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#define SCL_FS_PATHSIZE 1024

static const char* _normalizepath(const char* path) {
#if defined(_WIN32)
  return strreplacestr(path, "\\", "/");
#else
  return strcopy(path);
#endif
}

const char* scl_fs_exepath() {
  char buf[SCL_FS_PATHSIZE];
  size_t size = sizeof(buf);
  if(uv_exepath(buf, &size) < 0)
    return NULL;
  return _normalizepath(buf);
}

const char* scl_fs_exedir() {
  const char* ptr = scl_fs_exepath();
  if(!ptr)
    return NULL;
  const char* out = scl_parentpath(ptr);
  free((void*)ptr);
  return out;
}

const char* scl_fs_realpath(const char* path) {
  uv_fs_t req;
  if(uv_fs_realpath(uv_default_loop(), &req, path, NULL))
    return false;
  const char* out = _normalizepath(req.ptr);
  uv_fs_req_cleanup(&req);
  return out;
}

bool scl_fs_exists(const char* path) {
  uv_fs_t req;
  if(uv_fs_stat(uv_default_loop(), &req, path, NULL))
    return false;
  uv_fs_req_cleanup(&req);
  return true;
}

bool scl_fs_isdirectory(const char* path) {
  uv_fs_t req;
  if(uv_fs_stat(uv_default_loop(), &req, path, NULL))
    return false;

  bool r = S_ISDIR(req.statbuf.st_mode);
  uv_fs_req_cleanup(&req);
  return r;
}

bool scl_fs_isfile(const char* path) {
  uv_fs_t req;
  if(uv_fs_stat(uv_default_loop(), &req, path, NULL))
    return false;
  bool r = S_ISREG(req.statbuf.st_mode);
  uv_fs_req_cleanup(&req);
  return r;
}

int64_t scl_mtime(const char* path) {
  uv_fs_t req;
  if(uv_fs_stat(uv_default_loop(), &req, path, NULL))
    return false;
  int64_t r = req.statbuf.st_mtim.tv_sec;
  uv_fs_req_cleanup(&req);
  return r;
}

bool scl_fs_copy(const char* src, const char* dst) {
  uv_fs_t req;
  if(uv_fs_copyfile(uv_default_loop(), &req, src, dst, 0, NULL))
    return false;
  uv_fs_req_cleanup(&req);
  return true;
}
