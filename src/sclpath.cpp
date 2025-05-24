/*  sclpath.cpp
 *  Path class definitions for SCL
 */

#include "sclpath.hpp"
#include <sys/stat.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  define access _access
#  define F_OK   0
#else
#  include <dirent.h>
#  include <unistd.h>
#  ifdef __APPLE__
#    incldue < libproc.h>
#  endif
#endif

#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif

namespace scl {
path::path() {
}

path::path (string const &rhs) : string (rhs) {
}

path::path (char const *rhs) : string (rhs) {
}

path path::resolve() const {
  static char fpath[PATH_MAX];
#if defined(_WIN32)
  _fullpath (fpath, cstr(), PATH_MAX);
#elif defined(__unix__) || defined(__APPLE__)
  char *_ = ::resolve (cstr(), fpath);
#endif
  return fpath;
}

path path::parentpath() const {
  if (!*this)
    return "";
  auto        real = resolve();
  char const *abs  = real.cstr();
  int         l    = real.len();
  char       *p    = (char *)abs + l - 1;
  int         n    = -1;
  for (; *p && p >= abs; --p)
    if (*p == '/' || *p == '\\') {
      while (*p == '/' || *p == '\\')
        p--;
      p++;
      break;
    }
  n          = int (p - abs);
  string out = (p != abs) ? real.substr (0, n) : ".";
  return out;
}

path path::filename() const {
  auto c = split();
  if (c.size() > 0)
    return c.back();
  return "";
}

string path::extension() const {
  path file = filename();
  long p    = file.ffi (".");
  return p >= 0 ? file.substr (p) : "";
}

path path::basename() const {
  path file = filename();
  long p    = file.ffi (".");
  return p >= 0 ? file.substr (0, p) : file;
}

bool path::iswild() const {
  return ffi ("*") >= 0;
}

std::vector<path> path::split() const {
  std::vector<path> syms;
  char const       *s = cstr(), *p = cstr();
  while (*s && *p) {
    while (*p && *p != '/' && *p != '\\')
      p++;
    string sym = substr (unsigned (s - cstr()), unsigned (p - s));
    if (sym.ffi ("**") > 0)
      sym = "**";
    else if (!sym)
      sym = "";
    syms.push_back (sym);
    while (*p == '/' || *p == '\\')
      p++;
    s = p;
  }
  if (syms.size() == 1)
    syms = {".", syms[0]};
  return syms;
}

bool path::exists() const {
  int r = access (cstr(), F_OK) == 0;
  return r;
}

long path::wtime() const {
#if defined(__unix__) || defined(__APPLE__)
  struct stat s = {0};
  if (stat (cstr(), &s) == -1)
    return 0;
  return s.st_mtime;
#elif defined(_WIN32)
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
  return (long)ulint.QuadPart;
#endif
}

path &path::replaceFilename (path const &nFile) {
  auto c = split();
  if (c.size() > 0) {
    c.back() = nFile;
    *this    = join (c);
  }
  return *this;
}

path &path::replaceExtension (path const &nExt) {
  auto c = split();
  if (c.size() > 0) {
    auto &file = c.back();
    file.replace (file.extension(), nExt);
    *this = join (c);
  }
  return *this;
}

path &path::replaceBasename (path const &nName) {
  auto c = split();
  if (c.size() > 0) {
    auto &file = c.back();
    file.replace (file.basename(), nName);
    *this = join (c);
  }
  return *this;
}

path path::cwd() {
  return path (".").resolve();
}

path path::execdir() {
#ifdef _WIN32
  char buf[PATH_MAX + 1];
  memset (buf, 0, sizeof (buf));
  GetModuleFileName (NULL, buf, PATH_MAX - 1);
#elif defined(__APPLE__)
  char buf[PATH_MAX];
  proc_pidpath (getpid(), buf, PATH_MAX);
#else
  char    buf[PATH_MAX];
  ssize_t count = readlink ("/proc/self/exe", buf, PATH_MAX);
#endif
  path p = buf;
  return path (buf).parentpath();
}

bool path::mkdir (path const &path) {
  auto       dirs = path.split();
  class path dir;
  for (auto &i : dirs) {
    dir = dir / i;
    if (!dir.exists()) {
#if defined(__unix__) || defined(__APPLE__)
      struct stat s = {0};
      if (stat (path.cstr(), &s) == -1) {
        return false;
      }
      ::mkdir (path.cstr(), 0755);
#elif defined(_WIN32)
      if (!CreateDirectoryA (path.cstr(), NULL))
        return false;
#endif
    }
  }
  return true;
}

bool path::mkdir (std::vector<path> paths) {
  for (auto &i : paths) {
    if (!mkdir (i))
      return false;
  }
  return true;
}

bool path::chdir (path const &path) {
#if defined(_WIN32)
  return SetCurrentDirectory (path.cstr());
#elif defined(__unix__) || defined(__APPLE__)
  return !chdir (path.cstr());
#endif
}

static int glob_ (path const &dir, path const &mask, std::vector<string> &globs,
  bool files = true) {
#ifdef _WIN32
  // globs.reserve (128);
  string const     M     = mask;
  auto const       end   = mask.iswild() ? "*" : mask;
  string const     spec  = dir / end;
  HANDLE           hFind = NULL;
  WIN32_FIND_DATAA ffd;
  hFind = FindFirstFileA (spec.cstr(), &ffd);
  if (hFind == NULL || hFind == (HANDLE)0xffffffffLL)
    return 1;
  do {
    string fn = ffd.cFileName;
    if (fn == "." || fn == "..")
      continue;
    bool t = (files && !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ||
             (!files && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    if ((!mask.iswild() || string::match (fn.cstr(), M.cstr())) && t) {
      globs.push_back (dir / (mask.iswild() ? fn : mask));
    }
  } while (FindNextFile (hFind, &ffd) != 0);
  FindClose (hFind);
  return 0;
#elif defined(_DIRENT_H)
  /* clang-format off */
  DIR           *handle = opendir (dir.cstr());
  struct dirent *dp;
  while (handle) {
    if (!(dp = readdir (handle)))
      break;
    path fn = (char*)dp->d_name;
    if (fn == "." || fn == "..")
      continue;
    struct stat st;
    path        path = dir / fn;
    if (stat(path.cstr(), &st))
      continue;
    bool t = (files && S_ISREG(st.st_mode) || !files && S_ISDIR(st.st_mode));
    if (t && (!mask.iswild() || string::match(fn.cstr(), mask.cstr()))) {
      globs.push_back(dir / (mask.iswild() ? fn : mask));
    }
  }
  if (handle)
    closedir (handle);
  return 0;

  /* clang-format on */
#else
#  message "scl::io::glob missing implementation :("
  return 0;
#endif
}

static int glob_recurse (string const &mask, std::vector<string> &dirs,
  std::vector<string> &finds, bool misdir = true) {
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
  for (auto &i : dirs)
    finds.push_back (i);
  while (dirs.size() > 0) {
    std::vector<string> ndirs;
    for (auto &i : dirs)
      glob_ (i, "*", ndirs, false);
    if (misdir) {
      dirs.clear();
      for (auto &i : ndirs) {
        if (i.match (mask))
          finds.push_back (i);
        else
          dirs.push_back (i);
      }
    } else {
      for (auto &i : ndirs)
        finds.push_back (i);
    }
    dirs = ndirs;
  }
  return 0;
}

std::vector<string> path::glob (string const &pattern) {
  auto syms = path (pattern).split();
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
  path                glob;
  for (auto &sym : syms) {
    if (sym.iswild()) {
      if (glob)
        globs.push_back (glob);
      if (globs.empty())
        globs.push_back (".");
      globs.push_back (sym);
      glob.clear();
    } else {
      glob = glob / sym;
    }
  }
  if (glob && !glob.iswild())
    globs.push_back (glob);
  // For every dir glob, use previously expanded glob as a search dir (dirs),
  // then set the search dirs with the results (ndirs).
  // It is up to platform agnostic code to handle the bullshit that is **.
  std::vector<string> dirs = {globs[0]}, finds;
  // For ever dir glob
  for (int i = 1; i < globs.size() - 1; i++) {
    std::vector<string> ndirs;
    if (globs[i] == "**") {
      glob_recurse (globs[(long long)i + 1], dirs, ndirs, i < globs.size() - 2);
    } else {
      // For every search dir
      for (int j = 0; j < dirs.size(); j++)
        glob_ (dirs[j], globs[i], ndirs, false);
    }
    dirs = ndirs;
  }
  path fn = globs.back();
  // For ever search dir
  for (auto &dir : dirs)
    glob_ (dir, fn, finds);
  return finds;
}

path path::join (std::vector<path> components) {
  path out;
  for (auto &i : components) {
    out = out / i;
  }
  return out;
}

path path::operator/ (path const &rhs) const {
  string out;
  if (*this) {
    out.operator+= <PATH_MAX> (*this);
#ifdef _WIN32
    out += "\\";
#else
    out += "/";
#endif
  }
  out += rhs;
  return out;
}

} // namespace scl