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
#include <sclc_string.h>
#include <sclc_path.h>
#include "internal.hpp"
#include "scl_stream.hpp"
#include <sys/stat.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  define F_OK 0
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
  return path().claim(scl_pathabs(cstr(), nullptr));
}

bool path::haspath(const path& path) const {
  scl::path one = resolve();
  if(!one || !path)
    return false;
  return one.ffi(path);
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
      out.join(trimmed);
      return out;
    }
    out = out.join("..");
    if(!copy.isdirectory())
      break;
    copy = copy.parentpath();
  }
  return *this;
}

path path::parentpath() const {
  return path().claim(scl_parentpath(cstr()));
}

path path::filename() const {
  return path().claim(scl_filename(cstr()));
}

string path::extension() const {
  return path().claim(scl_pathext(cstr()));
}

scl::string path::stem() const {
  return scl::string().claim(scl_pathstem(cstr()));
}

bool path::iswild() const {
  return scl_iswild(cstr());
}

std::vector<path> path::split() const {
  std::vector<path> syms;
  const char* s = cstr();
  while(s && *s) {
    const char* component;
    component = scl_pathcomponent(&s);
    if(!component)
      break;
    syms.push_back(path().claim(component));
  }
  if(syms.size() == 1)
    syms = {".", syms[0]};
  return syms;
}

bool path::exists() const {
  return scl_pathexists(cstr());
}

bool path::isfile() const {
  return scl_isfile(cstr());
}

bool path::isdirectory() const {
  return scl_isdirectory(cstr());
}

bool path::isabsolute() const {
#ifdef _WIN32
  return match("*:*");
#else
  return match("/*");
#endif
}

long long path::wtime() const {
  return scl_wtime(cstr());
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
  return path().claim(scl_pathabs(".", nullptr));
}

path path::execdir() {
  return path().claim(scl_execdir());
}

bool path::mkdir(const path& path) {
  // Cannot create a path with an invalid name
  if(path.iswild())
    return false;
  auto dirs = path.split();
  class path dir;
  for(auto& i : dirs) {
    dir = dir / i;
    if(!dir.exists()) {
      if(!scl_mkdir(dir.cstr()))
        return false;
    }
  }
  return true;
}

bool path::copyfile(const path& from, const path& to) {
  scl::path::mkdir(to.parentpath());
  scl::stream src(from, OpenMode::READ, true);
  if(!src.is_open())
    return false;
  scl::stream dst(to, OpenMode::WRITE, true);
  if(!dst.is_open())
    return false;
  return dst.write(src);
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
    if(!scl_mkdir(i.cstr()))
      return false;
  }
  return true;
}

bool path::chdir(const path& path) {
  return scl_chdir(path.cstr());
}

static void join_(char* buf, const scl::string& one, const scl::string& two) {
  scl_pathjoinx(buf, one.cstr(), two.cstr());
}

std::vector<path> path::glob(const string& pattern, GlobMode mode) {
  const int cmode = mode == GlobMode::FILES ? SCL_GLOB_FILES : SCL_GLOB_DIRS;
  const char** finds = scl_glob(pattern.cstr(), NULL, cmode);
  if(!finds)
    return {};

  std::vector<path> out;
  const char** itr = finds;
  while(*itr) {
    out.push_back(path(*itr).copy());
    itr++;
  }
  scl_svfree(finds);
  return std::move(out);
}

path path::join(std::vector<path> components, bool ignoreback) {
  path out;
  for(auto& i : components) {
    if(i == ".")
      continue;
    if(i == "..") {
      if(!ignoreback) {
        if(!out.len() && out.filename() != ".." && out.filename() != ".")
          out.join(i);
        else
          out = out.parentpath();
        continue;
      }
    }
    out.join(i);
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

path& path::join(const path& rhs) {
  scl::path second;
  void* joined = nullptr;
  scl_pathjoin(joined, cstr(), rhs.cstr());
  if(!joined)
    throw std::runtime_error("failed to join paths");
  claim(strcopy((const char*)joined));
  return *this;
}

path path::operator/(const path& rhs) const {
  path out = *this;
  return std::move(out.join(rhs));
}

} // namespace scl
