#include <sclc_path.h>
#include <sclc_string.h>
#include <string.h>
#include <stdlib.h>
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
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  define access _access
#  define F_OK   0
#  pragma comment(lib, "Shlwapi.lib")
#  define _SLASH "\\"
#else
#  include <dirent.h>
#  include <unistd.h>
#  include <limits.h>
#  ifdef __APPLE__
#    include <libproc.h>
#  endif
#  define _SLASH "/"
#endif

#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif


int scl_pathjoinx(char* buf, const char* one, const char* two) {
  if(!buf || !one || !two)
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

const char* scl_pathabs(const char* path, char* resolved) {
  if(!path)
    return NULL;
  if(!resolved) {
    resolved = (char*)malloc(PATH_MAX + 1);
    memset(resolved, 0, PATH_MAX + 1);
  }
#if defined(_WIN32)
  _fullpath(resolved, path, PATH_MAX);
#else
  char* _ = realpath(path, resolved);
#endif
  return resolved;
}

const char* scl_execdir() {
  char buf[PATH_MAX + 1];
  memset(buf, 0, PATH_MAX + 1);
#ifdef _WIN32
  GetModuleFileNameA(NULL, buf, PATH_MAX);
#elif defined(__APPLE__)
  proc_pidpath(getpid(), buf, PATH_MAX);
#else
  ssize_t count = readlink("/proc/self/exe", buf, PATH_MAX);
#endif
  return scl_parentpath(buf);
}

bool scl_pathexists(const char* path) {
  bool r = access(path, F_OK) == 0;
  return r;
}

bool scl_isdirectory(const char* path) {
  if(!path)
    return 0;
#ifdef _WIN32
  DWORD fa = GetFileAttributesA(path);
  return (fa & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat st;
  if(stat(path, &st) == -1)
    return 0;
  return S_ISDIR(st.st_mode);
#endif
}

bool scl_isfile(const char* path) {
  if(!path)
    return 0;
#ifdef _WIN32
  return !scl_isdirectory(path);
#else
  struct stat st;
  if(stat(path, &st) == -1)
    return 0;
  return S_ISREG(st.st_mode);
#endif
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
      return strsub(p, 0, -1);
  }
  return NULL;
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
  char* ptr = (char*)malloc((size_t)i + 1);
  ptr[i] = 0;
  memcpy(ptr, path, (size_t)i);
  return 0;
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
  return strsub(s, 0, e - s);
}

bool scl_chdir(const char* path) {
  if(!path)
    return false;
#if defined(_WIN32)
  return !_chdir(path);
#else
  return !chdir(path);
#endif
}

bool scl_mkdir(const char* path) {
  if(!scl_isdirectory(path)) {
#if defined(_WIN32)
    if(_mkdir(path))
      return false;
#else
    if(mkdir(path, 0777))
      return false;
#endif
  }
  return true;
}

bool scl_mkdirs(const char** paths, int count) {
  if(!paths || count <= 0)
    return false;
  int i;
  for(i = 0; i < count; i++) {
    const char* path = paths[i];
    if(!path)
      return false;
    if(!scl_mkdir(path))
      return false;
  }
  return true;
}

static const char** glob_unit(
  const char* dir, const char* mask, const char** finds, int mode) {
  char buf[PATH_MAX];
  const bool wildmask = scl_iswild(mask);

#ifdef _WIN32
  const char* specmask = (wildmask ? "*" : mask);
  const char* spec = NULL;
  scl_pathjoin(spec, dir, specmask);
  HANDLE hFind = NULL;
  WIN32_FIND_DATAA ffd;
  hFind = FindFirstFileA(spec, &ffd);
  if(hFind == NULL || hFind == (HANDLE)0xffffffffLL)
    return finds;
  do {
    const char* fn = ffd.cFileName;
#elif defined(_DIRENT_H) || defined(_SYS_DIRENT_H)
  DIR* handle = opendir(dir);
  struct dirent* dp;
  while(handle) {
    if(!(dp = readdir(handle)))
      break;
    const char* fn = (char*)dp->d_name;
#endif

    if(!strcmp(fn, ".") || !strcmp(fn, ".."))
      continue;
    scl_pathjoinx(buf, dir, fn);
    bool validt = true;
    if(mode == SCL_PATH_FILES && !scl_isfile(buf))
      validt = false;
    else if(mode == SCL_PATH_DIRS && !scl_isdirectory(buf))
      validt = false;
    if(validt && (!wildmask || strmatch(mask, fn))) {
      scl_svappend(finds, buf, 2048);
    }

#ifdef _WIN32
  } while(FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);
#elif defined(_DIRENT_H) || defined(_SYS_DIRENT_H)
  }
  if(handle)
    closedir(handle);
#else
#  pragma message("glob_unit has no implementation for this platform")
#endif

  return finds;
}

static const char** glob_recurse(const char* mask, const char** dirs) {
  // Expand each in dirs, output into dirs.
  const char** searches = dirs;
  dirs = NULL;
  for(size_t i = 0; *searches[i]; i++) {
    searches = glob_unit(searches[i], "*", searches, SCL_PATH_DIRS);
    if(strmatch(mask, scl_filename(searches[i]))) {
      scl_svappend(dirs, searches[i], 1024);
    }
  }
  scl_svfree(searches);
  return dirs;
}

static const char** glob_singlepattern(
  const char** finds, const char* pattern, int mode) {
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
  const char** globs = NULL;
  char glob[PATH_MAX] = {0};
  const char* sym = scl_pathcomponent(&pattern);
  while(sym) {
    if(scl_iswild(sym)) {
      if(glob[0])
        scl_svappend(globs, glob, 128);
      if(!globs)
        scl_svappend(globs, ".", 128);
      scl_svappend(globs, sym, 128);
      glob[0] = 0;
    } else {
      scl_pathjoinx(glob, glob, sym);
    }
    free((void*)sym);
    sym = scl_pathcomponent(&pattern);
  }
  if(glob[0] && !scl_iswild(glob))
    scl_svappend(globs, glob, 128);
  /* For every glob expression from second element and up, use previously
   * expanded expression as a search dir (dirs), then set the search dirs with
   * the results (ndirs).
   */
  const char** dirs = NULL;
  scl_svappend(dirs, globs[0], 1024);
  /* For every dir glob */
  for(size_t i = 1; i < scl_svlen(globs); i++) {
    if(!strncmp(globs[i], "**", 3)) {
      const char* mask = "*";
      // If not the last glob exp, use the next exp as the mask
      if(i < scl_svlen(globs) - 1)
        mask = globs[i];
      dirs = glob_recurse(mask, dirs);
      i++;
      /* If not the last glob exp */
    } else if(i < scl_svlen(globs) - 1) {
      // Find new search dirs
      for(size_t j = 0; dirs[j]; j++)
        dirs = glob_unit(dirs[j], globs[i], dirs, SCL_PATH_DIRS);
    }
  }
  const char* fn = scl_svback(globs);
  // Find requested items
  for(size_t i = 0; i < scl_svlen(dirs); i++)
    finds = glob_unit(dirs[i], fn, finds, mode);
  scl_svfree(globs);
  scl_svfree(dirs);
  return finds;
}

const char** scl_glob(const char* pattern, const char** finds, int mode) {
  if(!pattern)
    return NULL;
  const char *s = pattern, *p = s;
  while(true) {
    p = strstr(s, ";");
    if(!p) {
      finds = glob_singlepattern(finds, s, mode);
      break;
    }
    const char* search = strsub(s, 0, p - s);
    finds = glob_singlepattern(finds, search, mode);
    s = p + 1;
  }
  return finds;
}

int64_t scl_wtime(const char* path) {
  if(!path)
    return 0;
  struct stat s = {0};
  if(stat(path, &s) == -1)
    return 0;
  return s.st_mtime;
}
