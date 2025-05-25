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
#  include <shlwapi.h>
#  define access _access
#  define F_OK   0
#  pragma comment(lib, "Shlwapi.lib")
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
  char       *_ = realpath (cstr(), fpath);
#endif
  return path (fpath).copy();
}

bool path::haspath (path const &path) const {
  scl::path copy = resolve();
  return copy.ffi (path.resolve()) >= 0;
}

static path trimpath (path const &path, scl::path const &with) {
  std::vector<scl::path> comp;
  auto                   frc = with.split();
  auto                   fic = path.split();
  for (int i = 0; i < fic.size(); i++) {
    if (i < frc.size() && frc[i] == fic[i])
      continue;
    comp.push_back (fic[i]);
  }
  return path::join (comp);
}

path path::relative (path const &from) const {
  scl::path copy = from.resolve();
  scl::path out;
  while (copy) {
    if (haspath (copy)) {
      auto trimmed = trimpath (*this, copy);
      out.join (trimmed, false);
      return out;
    }
    out = out.join ("..", false);
    if (!copy.isdirectory())
      break;
    copy = copy.parentpath();
  }
  return *this;
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

path path::stem() const {
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

bool path::isfile() const {
  if (!exists())
    return false;
#ifdef _WIN32
  return !isdirectory();
#else
  struct stat st;
  if (stat (cstr(), &st) == -1)
    return false;
  return S_ISREG (st.st_mode);
#endif
}

bool path::isdirectory() const {
  if (!exists())
    return false;
#ifdef _WIN32
  DWORD fa = GetFileAttributesA (cstr());
  return (fa & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st;
  if (stat (cstr(), &st) == -1)
    return false;
  return S_ISDIR (st.st_mode);
#endif
}

long long path::wtime() const {
  struct stat s = {0};
  if (stat (cstr(), &s) == -1)
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
  return (unsigned long)ulint.QuadPart;
#  endif
#endif
}

void path::remove() const {
  if (iswild() && isfile())
    return;
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

path &path::replaceStem (path const &nName) {
  auto c = split();
  if (c.size() > 0) {
    auto &file = c.back();
    file.replace (file.stem(), nName);
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

bool path::copyfile (path const &from, path const &to) {
  std::ifstream in (from.cstr(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open())
    return false;
  std::ofstream out (to.cstr(), std::ios_base::out | std::ios_base::binary);
  if (!out.is_open())
    return false;
  scl::string contents;
  in >> contents;
  out.write (contents.cstr(), contents.size());
  in.close();
  out.close();
  return true;
}

bool path::movefile (path const &from, path const &to) {
#ifdef _WIN32
  return !!MoveFileA (from.cstr(), to.cstr());
#else
  return !rename (from.cstr(), to.cstr());
#endif
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

static int glob_ (path const &dir, path const &mask, std::vector<path> &globs,
  bool files = true) {
#ifdef _WIN32
  path const       spec  = dir / (mask.iswild() ? "*" : mask);
  HANDLE           hFind = NULL;
  WIN32_FIND_DATAA ffd;
  hFind = FindFirstFileA (spec.cstr(), &ffd);
  if (hFind == NULL || hFind == (HANDLE)0xffffffffLL)
    return 1;
  do {
    path fn = ffd.cFileName;
#elif defined(_DIRENT_H)
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
    path path = dir / (mask.iswild() ? fn : mask);
    bool t    = (files && path.isfile()) || (!files && path.isdirectory());
    if (t && (!mask.iswild() || string::match (fn.cstr(), mask.cstr()))) {
      globs.push_back (path);
    }
#ifdef _WIN32
  } while (FindNextFile (hFind, &ffd) != 0);
  FindClose (hFind);
#elif defined(_DIRENT_H)
  }
  if (handle)
    closedir (handle);

/* clang-format on */
#else
#  message "scl::io::glob missing implementation :("
#endif
  return 0;
}

static int glob_recurse (string const &mask, std::vector<path> &dirs,
  std::vector<path> &finds, bool misdir = true) {
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
    std::vector<path> ndirs;
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

std::vector<path> path::glob (string const &pattern) {
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
  std::vector<path> dirs = {globs[0]}, finds;
  // For ever dir glob
  for (int i = 1; i < globs.size() - 1; i++) {
    std::vector<path> ndirs;
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

path path::join (std::vector<path> components, bool ignoreback) {
  path out;
  for (auto &i : components) {
    if (i == ".")
      continue;
    if (i == "..") {
      if (!ignoreback) {
        if (!out.len() && out.filename() != ".." && out.filename() != ".")
          out.join (i, false);
        else
          out = out.parentpath();
        continue;
      }
    }
    out.join (i, false);
  }
  return out;
}

path &path::join (path const &rhs, bool relative) {
  if (*this) {
#ifdef _WIN32
    this->operator+=<64> ("\\");
#else
    this->operator+=<64> ("/");
#endif
  }
  if (relative)
    this->operator+=<64> (rhs.relative (*this));
  else
    this->operator+=<64> (rhs);
  return *this;
}

path path::operator/ (path const &rhs) const {
  path out = *this;
  return out.join (rhs);
}

} // namespace scl