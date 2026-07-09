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


/*  path.cpp
 *  Path class definitions for SCL
 */

#include <scl_path.hpp>
#include "internal.hpp"
#include <sys/stat.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  include <shlwapi.h>
#  define access _access
#  define F_OK   0
#  pragma comment(lib, "Shlwapi.lib")
#else
#  include <dirent.h>
#  include <unistd.h>
#  ifdef __APPLE__
#    include <libproc.h>
#  endif
#endif

namespace scl {
path::path() {
}

path::path(const string& rhs) : string(rhs) {
  replace("\\", "/");
}

path::path(const char* rhs) : string(rhs) {
  replace("\\", "/");
}

path& path::fixendsplit() {
  int32_t p = len() - 1;
  for(char c; p != -1 && (c = (*this)[p]) && (c == '/' || c == '\\'); p--) {
  }
  if(p != len() - 1)
    *this = substr(0, p + 1);
  return *this;
}

path path::resolve() const {
  if(isabsolute())
    return *this;
  static char fpath[PATH_MAX];
#if defined(_WIN32)
  _fullpath(fpath, cstr(), PATH_MAX);
#elif defined(__unix__) || defined(__APPLE__)
  char* _ = realpath(cstr(), fpath);
#endif
  return path(fpath).copy();
}

bool path::haspath(const path& path) const {
  scl::path fixed = path.resolve().fixendsplit();
  return resolve().ffi(fixed) >= 0;
}

static path trimpath(const path& path, const scl::path& with) {
  std::vector<scl::path> comp;
  auto frc = with.split();
  auto fic = path.split();
  for(size_t i = 0; i < fic.size(); i++) {
    if(i < frc.size() && frc[i] == fic[i])
      continue;
    comp.push_back(fic[i]);
  }
  return path::join(comp);
}

path path::relative(const path& base) const {
  if(!isabsolute())
    return *this;
  scl::path copy = base.resolve();
  scl::path out;
  while(copy) {
    if(haspath(copy)) {
      auto trimmed = trimpath(*this, copy);
      out.join(trimmed, false);
      return out;
    }
    out = out.join("..", false);
    if(!copy.isdirectory())
      break;
    copy = copy.parentpath();
  }
  return *this;
}

path path::parentpath() const {
  if(!*this)
    return "";
  auto real = resolve();
  const char* abs = real.cstr();
  int32_t l = real.len();
  if(!l)
    return "";
  char* p = (char*)abs + l - 1;
  int32_t n = -1;
  for(; *p && p >= abs; --p)
    if(*p == '/' || *p == '\\') {
      while(*p == '/' || *p == '\\')
        p--;
      p++;
      break;
    }
  n = (int32_t)(p - abs);
  string out = (p != abs) ? real.substr(0, n) : ".";
  return out;
}

path path::filename() const {
  int32_t p = std::max(fli("/"), fli("\\"));
  if(p == -1)
    return *this;
  return substr(p + 1);
}

string path::extension() const {
  path file = filename();
  int32_t p = file.ffi(".");
  return p >= 0 ? file.substr(p) : "";
}

path path::stem() const {
  path file = filename();
  int32_t p = file.ffi(".");
  return p >= 0 ? file.substr(0, p) : file;
}

bool path::iswild() const {
  return ffi("*") >= 0;
}

std::vector<path> path::split() const {
  std::vector<path> syms;
  const char *s = cstr(), *p = cstr();
  if(!s)
    return syms;
  while(*s && *p) {
    while(*p && *p != '/' && *p != '\\')
      p++;
    string sym = substr((int32_t)(s - cstr()), (int32_t)(p - s));
    if(sym.ffi("**") > 0)
      sym = "**";
    else if(!sym)
      sym = "";
    syms.push_back(sym);
    while(*p == '/' || *p == '\\')
      p++;
    s = p;
  }
  if(syms.size() == 1)
    syms = {".", syms[0]};
  return syms;
}

bool path::exists() const {
  int r = access(cstr(), F_OK) == 0;
  return !!r;
}

bool path::isfile() const {
  if(!exists())
    return false;
#ifdef _WIN32
  return !isdirectory();
#else
  struct stat st;
  if(stat(cstr(), &st) == -1)
    return false;
  return S_ISREG(st.st_mode);
#endif
}

bool path::isdirectory() const {
  if(!exists())
    return false;
#ifdef _WIN32
  DWORD fa = GetFileAttributesA(cstr());
  return (fa & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st;
  if(stat(cstr(), &st) == -1)
    return false;
  return S_ISDIR(st.st_mode);
#endif
}

bool path::isabsolute() const {
#ifdef _WIN32
  return match("*:*");
#else
  return match("/*");
#endif
}

long long path::wtime() const {
  struct stat s = {0};
  if(stat(cstr(), &s) == -1)
    return 0;
  return s.st_mtime;
#if 0
#  if defined(__unix__) || defined(__APPLE__)
#  elif defined(_WIN32)
  FILETIME ftCreate, ftAccess, ftWrite;
  OFSTRUCT of;
  HFILE    hf = OpenFile (cstr(), &of, OF_READ);
  if (hf == HFILE_ERROR)
    return 0;
  if (!GetFileTime ((HANDLE)(intptr_t)hf, &ftCreate, &ftAccess, &ftWrite)) {
    CloseHandle ((HANDLE)(intptr_t)hf);
    return 0;
  }
  ULARGE_INTEGER ulint;
  ulint.LowPart  = ftWrite.dwLowDateTime;
  ulint.HighPart = ftWrite.dwHighDateTime;
  CloseHandle ((HANDLE)(intptr_t)hf);
  return (int32_t long)ulint.QuadPart;
#  endif
#endif
}

void path::remove() const {
  if(iswild() && isfile())
    return;
}

path& path::replaceFilename(const path& nFile) {
  auto c = split();
  if(c.size() > 0) {
    c.back() = nFile;
    *this = join(c);
  }
  return *this;
}

path& path::replaceExtension(const path& nExt) {
  auto p = fli(".");
  if(p == -1)
    return *this;
  replace(nExt, (int)p);
  return *this;
}

path& path::replaceStem(const path& nName) {
  auto c = split();
  if(c.size() > 0) {
    auto& file = c.back();
    file.replace(file.stem(), nName);
    *this = join(c);
  }
  return *this;
}

path path::cwd() {
  return path(".").resolve();
}

path path::execdir() {
#ifdef _WIN32
  char buf[PATH_MAX + 1];
  memset(buf, 0, sizeof(buf));
  GetModuleFileName(NULL, buf, PATH_MAX - 1);
#elif defined(__APPLE__)
  char buf[PATH_MAX];
  proc_pidpath(getpid(), buf, PATH_MAX);
#else
  char buf[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", buf, PATH_MAX);
#endif
  path p = buf;
  return path(buf).parentpath();
}

bool path::mkdir(const path& path) {
  auto dirs = path.split();
  class path dir;
  for(auto& i : dirs) {
    dir = dir / i;
    if(!dir.exists()) {
#if defined(__unix__) || defined(__APPLE__)
      /*struct stat s = {0};
      if(stat(dir.cstr(), &s) == -1) {
        return false;
      }*/
      ::mkdir(dir.cstr(), 0755);
#elif defined(_WIN32)
      if(!CreateDirectoryA(dir.cstr(), NULL))
        return false;
#endif
    }
  }
  return true;
}

bool path::copyfile(const path& from, const path& to) {
  scl::path::mkdir(to.parentpath());
  std::ifstream in(from.cstr(), std::ios_base::in | std::ios_base::binary);
  if(!in.is_open())
    return false;
  std::ofstream out(to.cstr(), std::ios_base::out | std::ios_base::binary);
  if(!out.is_open())
    return false;
  scl::string contents;
  in >> contents;
  out.write(contents.cstr(), contents.size());
  in.close();
  out.close();
  return true;
}

bool path::movefile(const path& from, const path& to) {
#ifdef _WIN32
  return !!MoveFileA(from.cstr(), to.cstr());
#else
  return !rename(from.cstr(), to.cstr());
#endif
}

bool path::mkdir(std::vector<path> paths) {
  for(auto& i : paths) {
#ifdef _WIN32
    if(!CreateDirectoryA(i.cstr(), NULL))
      return false;
#else
    if(!::mkdir(i.cstr(), 0777))
      return false;
#endif
  }
  return true;
}

bool path::chdir(const path& path) {
#if defined(_WIN32)
  return SetCurrentDirectory(path.cstr());
#elif defined(__unix__) || defined(__APPLE__)
  return !::chdir(path.cstr());
#endif
}

template <typename T>
static void align_reserve(std::vector<T>& vec, int add, int align = 128) {
  if(vec.size() + add > vec.capacity()) {
    add = (add / (align + 1) + 1) * align;
    vec.reserve(vec.capacity() + add);
  }
}

static void join_(char* buf, const scl::string& one, const scl::string& two) {
  const int32_t l1 = one.len();
  int32_t l2 = two.len();
  l2 = std::min(l2, l1 + l2 + 2 - PATH_MAX);
  memcpy(buf, one.cstr(), (size_t)l1);
  buf[l1] = '/';
  memcpy(buf + l1 + 1, two.cstr(), (size_t)l2);
  buf[l1 + l2 + 1] = 0;
}

static int glob_(const path dir, const path& mask, std::vector<path>& globs,
  scl::GlobMode mode = scl::GlobMode::FILES) {
  char buf[PATH_MAX];
#ifdef _WIN32
  const path spec = dir / (mask.iswild() ? "*" : mask);
  HANDLE hFind = NULL;
  WIN32_FIND_DATAA ffd;
  hFind = FindFirstFileA(spec.cstr(), &ffd);
  if(hFind == NULL || hFind == (HANDLE)0xffffffffLL)
    return 1;
  do {
    path fn = ffd.cFileName;
#elif defined(_DIRENT_H) || defined(_SYS_DIRENT_H)
  /* clang-format off */
  DIR           *handle = opendir (dir.cstr());
  struct dirent *dp;
  while (handle) {
    if (!(dp = readdir (handle)))
      break;
    path fn = (char*)dp->d_name;
#endif
    if (fn == "." || fn == "..")
      continue;
    join_(buf, dir, (mask.iswild() ? fn : mask));
    path path;
    path.view(buf);
    bool validt = true;
    if (mode == scl::GlobMode::FILES && !path.isfile())
      validt = false;
    else if (mode == scl::GlobMode::DIRS && !path.isdirectory())
      validt = false;
    if (validt && (!mask.iswild() || string::match (fn.cstr(), mask.cstr()))) {
      align_reserve(globs, 1, 256);
      globs.push_back (path.copy());
    }
#ifdef _WIN32
  } while (FindNextFile (hFind, &ffd) != 0);
  FindClose (hFind);
#elif defined(_DIRENT_H) || defined(_SYS_DIRENT_H)
  }
  if (handle)
    closedir (handle);

/* clang-format on */
#else
#  pragma message("scl::io::glob has no implementation for this platform")
#endif
  return 0;
}

static int glob_recurse(const string& mask, std::vector<path>& dirs) {
  // For each in dirs, add to finds.
  /* LOOP
    For each in dirs, search for every directory (ndirs).

    IF misdir IS TRUE
    Clear dirs.
    For each in ndirs, match them to mask.
    If they match, add them to finds, otherwise add to dirs.
    Set dirs to ndirs. Repeat until dirs is empty.

    IF misdir IS FALSE
    For each in ndirs, add to finds.
    Set dirs to ndirs. Repeat until dirs is empty.
  */
  std::vector<path> searches = dirs;
  dirs.clear();
  for(size_t i = 0; i < searches.size(); i++) {
    glob_(searches[i], "*", searches, GlobMode::DIRS);
    if(searches[i].filename().match(mask)) {
      align_reserve(dirs, 4);
      dirs.push_back(std::move(searches[i]));
    }
  }
  return 0;
}

static void glob_singlepattern(
  std::vector<path>& finds, const string& pattern, GlobMode mode) {
  auto syms = path(pattern).split();
  /* LOOP (for each in syms)
    IF sym IS WILDCARD
      IF glob IS NOT EMPTY
        Add glob to globs.
      IF globs IS EMPTY
        Add . to globs.
      Add sym to globs.
      Clear glob.
    ELSE
      Add sym to glob (glob = glob/sym).
  */
  // IF glob IS VALID AND glob IS NOT WILD
  //   Add glob to globs.
  std::vector<string> globs;
  path glob;
  for(auto& sym : syms) {
    if(sym.iswild()) {
      if(glob)
        globs.push_back(glob);
      if(globs.empty())
        globs.push_back(".");
      globs.push_back(sym);
      glob.clear();
    } else {
      glob = glob / sym;
    }
  }
  if(glob && !glob.iswild())
    globs.push_back(glob);
  // For every glob expression from second element and up, use previously
  // expanded expression as a search dir (dirs), then set the search dirs with
  // the results (ndirs).
  std::vector<path> dirs = {globs[0]};
  // For every dir glob
  for(size_t i = 1; i < globs.size(); i++) {
    if(globs[i] == "**") {
      scl::string mask = "*";
      // If not the last glob exp, use the next exp as the mask
      if(i < globs.size() - 1)
        mask = globs[i + 1];
      glob_recurse(mask, dirs);
      i++;
      // If not the last glob exp
    } else if(i != globs.size() - 1) {
      // Find new search dirs
      for(size_t j = 0; j < dirs.size(); j++)
        glob_(dirs[j], globs[i], dirs, GlobMode::DIRS);
    }
  }
  path fn = globs.back();
  // Find requested items
  for(auto& dir : dirs)
    glob_(dir, fn, finds, mode);
}

std::vector<path> path::glob(const string& pattern, GlobMode mode) {
  std::vector<string> searches;
  const char* sp = pattern.cstr();
  while(true) {
    scl::string tmp = sp;
    int32_t p = tmp.ffi(";");
    if(p <= 0) {
      searches.push_back(tmp);
      break;
    }
    searches.push_back(tmp.substr(0, p));
    sp += p + 1;
  }
  std::vector<path> finds;
  for(const auto& search : searches)
    glob_singlepattern(finds, search, mode);
  return std::move(finds);
}

path path::join(std::vector<path> components, bool ignoreback) {
  path out;
  for(auto& i : components) {
    if(i == ".")
      continue;
    if(i == "..") {
      if(!ignoreback) {
        if(!out.len() && out.filename() != ".." && out.filename() != ".")
          out.join(i, false);
        else
          out = out.parentpath();
        continue;
      }
    }
    out.join(i, false);
  }
  return out;
}

std::vector<path> path::splitPaths(const scl::string& paths) {
  const char *ps = paths.cstr(), *s = ps;
  std::vector<path> out;
  while(*ps) {
    int32_t p = scl::string::ffi(ps, ";");
    if(p < 0)
      p = (int32_t)strlen(ps);
    out.push_back(paths.substr((int32_t)(ps - s), p));
    ps += p + 1;
  }
  return std::move(out);
}

path& path::join(const path& rhs, bool relative) {
  scl::path second;

  if(*this) {
    char c = this->len() > 0 ? this->cstr()[this->len() - 1] : 0;
    if(c != '/')
      this->operator+= <64>("/");
  }
  if(relative && !!*this)
    this->operator+= <64>(rhs.relative(*this));
  else
    this->operator+= <64>(rhs);
  fixendsplit();
  return *this;
}

path path::operator/(const path& rhs) const {
  path out = *this;
  return std::move(out.join(rhs));
}

} // namespace scl
