/* string.cpp
 * scl::string class
 * utf-8 string container
 */

#include <scl_string.hpp>
#include <scl_misc.hpp>
#include "internal.hpp"
#include "lz4/xxhash.h"

#ifdef _WIN32
#  include <windows.h>
#endif

#define _plen(len) ((len) >= 0 ? (len) : 0)
#define _mlen(len) ((len) >= 0 ? (len) : INT_MAX)

namespace scl {
bool string::isview() const {
  return m_buf && !m_sz;
}

void string::make_unique() {
  if(!isview() || !*this)
    return;
  char* buf = new char[(size_t)m_ln + 1];
  memcpy(buf, m_buf, (size_t)m_ln + 1);
  m_buf = buf;
  m_sz = m_ln;
}

string::string() {
}

string::string(const std::string& str) {
  *this = string(str.c_str()).copy();
}

string::string(const char* str) {
  view(str);
}

#ifdef _WIN32
string::string(const wchar_t* wstr) {
  m_sz = 0;
  m_buf = nullptr;
  if(wstr) {
    int n =
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr) +
      1;
    reserve((int32_t)n);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, m_buf, n, nullptr, nullptr);
  }
}
#endif

string::string(const string& rhs) {
  if(rhs.m_buf) {
    if(rhs.isview()) {
      m_buf = rhs.m_buf;
      m_ln = rhs.m_ln;
      m_sz = rhs.m_sz;
    } else {
      m_sz = rhs.size();
      m_buf = new char[(size_t)m_sz + 1];
      memcpy(m_buf, rhs.m_buf, (size_t)m_sz + 1);
      m_ln = rhs.m_ln;
    }
  }
}

string::~string() {
  if(!isview() && *this) {
    delete[] m_buf;
  }
}

string& string::operator=(const string& rhs) {
  clear();
  if(rhs) {
    if(rhs.isview()) {
      m_buf = rhs.m_buf;
      m_ln = rhs.m_ln;
      m_sz = rhs.m_sz;
    } else {
      m_sz = rhs.size();
      m_buf = new char[(size_t)m_sz + 1];
      memcpy(m_buf, rhs.m_buf, (size_t)m_sz + 1);
      m_ln = rhs.m_ln;
    }
  }
  return *this;
}

void string::clear() {
  if(!isview() && *this)
    delete[] m_buf;
  m_buf = nullptr;
  m_ln = 0;
  m_sz = 0;
}

string& string::claim(const char* ptr) {
  clear();
  m_buf = (char*)ptr;
  m_ln = ptr ? (int32_t)strlen(ptr) : 0;
  m_sz = m_ln;
  return *this;
}

string& string::view(const char* ptr) {
  // Become untracked, as we are only viewing
  clear();
  m_buf = (char*)ptr;
  m_ln = ptr ? (int32_t)strlen(ptr) : 0;
  return *this;
}

string& string::reserve(int32_t newSize) {
  newSize = _plen(newSize);
  if(newSize == 0) {
    clear();
    return *this;
  }
  char* nbuf = new char[(size_t)newSize + 1];
  if(!nbuf)
    throw "out of stream";
  memset(nbuf, 0, (size_t)newSize + 1);
  if(*this)
    memcpy(nbuf, m_buf, (size_t)std::min(size(), newSize));
  claim(nbuf);
  m_ln = len();
  m_sz = newSize;
  return *this;
}

const char* string::cstr() const {
  return m_buf;
}
#ifdef _WIN32
const wchar_t* string::wstr() const {
  if(!m_buf)
    return nullptr;
  int wlen = MultiByteToWideChar(CP_UTF8, 0, m_buf, -1, nullptr, 0);
  int wsize = (wlen + 1);
  wchar_t* wstr = new wchar_t[(size_t)wsize];
  memset(wstr, 0, sizeof(wchar_t) * wsize);
  MultiByteToWideChar(CP_UTF8, 0, m_buf, -1, wstr, wlen);
  return wstr;
}
#endif

#define _isHex(c) ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
#define _fromHex(c)                        \
  ((c >= 'A' && c <= 'F') ? (c - 'A' + 10) \
                          : ((c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0))

int64_t string::toInt() const {
  if(!*this)
    return 0;
  bool hex = this->operator[](0) == '0' && this->operator[](1) == 'x';
  int64_t o = 0;
  for(int32_t i = hex ? 2 : 0; i < len(); i++) {
    char c = m_buf[i];
    int m = 0;
    if((c >= '0' && c <= '9'))
      m = c - '0';
    else if(hex && _isHex(c))
      m = _fromHex(c);
    else
      break;
    o = o * (hex ? 16 : 10) + m;
  }
  return o;
}

int32_t string::len() const {
  return m_ln;
}

int32_t string::size() const {
  if(!isview())
    return m_sz;
  else
    return m_ln;
}

int32_t string::ffi(const string& pattern) const {
  if(!*this || !pattern)
    return -1;
  const char* p = m_buf;
  int32_t csl = pattern.len();
  for(; p < m_buf + m_ln && *p; p++) {
    if(!strncmp(p, pattern.cstr(), (size_t)csl))
      return (int32_t)(p - m_buf);
  }
  return -1;
}

int32_t string::fli(const string& pattern) const {
  if(!*this || !pattern)
    return -1;
  const int32_t l = m_ln;
  int32_t csl = pattern.m_ln;
  const char* p = m_buf + l - csl;
  for(; p >= m_buf && *p; p--) {
    if(!strncmp(p, pattern.m_buf, (size_t)csl))
      return (int32_t)(p - m_buf);
  }
  return -1;
}

bool string::endswith(const string& pattern) const {
  int32_t p = fli(pattern);
  return p > 0 && p == m_ln - pattern.m_ln;
}

static bool str_match(
  const char* pattern, const char* candidate, int32_t p, int32_t c) {
  if(pattern[p] == '\0') {
    return candidate[c] == '\0';
  } else if(pattern[p] == '*') {
    for(;; c++) {
      if(candidate[c] == '\0' || candidate[c] == pattern[p + 1])
        break;
    }
    return str_match(pattern, candidate, p + 1, c);
  } else if(candidate[c] == pattern[p]) {
    return str_match(pattern, candidate, p + 1, c + 1);
  }
  return false;
}

bool string::match(const string& pattern) const {
  if(!*this || !pattern)
    return 0;
  return (bool)str_match(pattern.cstr(), cstr(), 0, 0);
}

uint64_t string::hash() const {
  if(!*this)
    return 0;
  uint64_t h = XXH64(m_buf, (size_t)m_ln, 1024);
  return h;
}

string string::substr(int32_t i, int32_t j) const {
  if(!*this || i >= m_ln)
    return "";
  i = _plen(i); // i >= 0
  j = _mlen(j); // j >= 0
  j = std::min(j, (int32_t)strlen(m_buf + i));
  char* out = new char[(size_t)j + 1];
  if(!out)
    throw "out of stream";
  memset(out, 0, (size_t)j + 1);
  memcpy(out, m_buf + i, (size_t)j);
  string sout;
  sout.claim(out);
  return std::move(sout);
}

string& string::replace(const string& pattern, const string& with) {
  if(!*this || !pattern)
    return *this;
  const char* str = m_buf;
  string out;
  while(1) {
    string tstr = str;
    int32_t p = tstr.ffi(pattern);
    if(p < 0)
      break;
    replace(with, p, pattern.len());
    str = m_buf;
  }
  return *this;
}

scl::string& string::replace(const scl::string& with, int32_t i, int32_t j) {
  if(!*this || i >= m_ln)
    return *this;
  i = _plen(i); // i >= 0
  j = _mlen(j); // j >= 0
  auto l = with.len();
  j = std::min(j, m_ln - i);
  int32_t d = len() - i - j;
  if(len() - j + l > size())
    reserve(i + l + d);
  else if(isview())
    make_unique();
  if(d)
    memcpy(m_buf + i + l, m_buf + i + j, (size_t)d);
  memcpy(m_buf + i, with.cstr(), (size_t)l);
  m_buf[i + l + d] = 0;
  m_ln = i + l + d;
  return *this;
}

string& string::toUpper() {
  for(auto& c : *this) {
    if(c >= 'a' && c <= 'z') {
      c -= 32;
    }
  }
  return *this;
}

string& string::toLower() {
  for(auto& c : *this) {
    if(c >= 'A' && c <= 'Z') {
      c += 32;
    }
  }
  return *this;
}

int32_t string::ffi(const char* str, const char* pattern) {
  if(!str || !pattern)
    return -1;
  scl::string S(str); // View str
  return S.ffi(pattern);
}

int32_t string::fli(const char* str, const char* pattern) {
  if(!str || !pattern)
    return -1;
  scl::string S(str); // View str
  return S.fli(pattern);
}

string string::substr(const char* str, int32_t i, int32_t j) {
  if(!str)
    return "";
  scl::string S(str); // View str;
  return S.substr(i, j);
}

string string::copy() const {
  return substr(0);
}

string string::rand(int32_t len) {
  static const char rchars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  string str;
  len = len < 0 ? 0 : len;
  str.reserve(len);
  int32_t i;
  for(i = 0; i < len; i++) {
    str[i] = rchars[rand_int(0, sizeof(rchars) - 1)];
    str.m_ln++;
  }
  return str;
}

string string::vfmt(const char* fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  int size = vsnprintf(nullptr, 0, fmt, copy);
  size = _mlen(size) + 1;
  va_end(copy);
  char* str = new char[(size_t)size];
  if(!str)
    throw "out of stream";
  memset(str, 0, (size_t)size);
  vsnprintf(str, (size_t)size, fmt, args);
  string out;
  out.claim(str);
  return out;
}

string string::fmt(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  string out = vfmt(fmt, args);
  va_end(args);
  return out;
}

uint64_t string::hash(const string& str) {
  return str.hash();
}

bool string::match(const char* str, const char* pattern) {
  if(!str || !pattern)
    return 0;
  return str_match(pattern, str, 0, 0);
}

_str_iterator string::begin() {
  if(!*this)
    return _str_iterator();
  return _str_iterator(*this, 0);
}

_str_const_iterator string::cbegin() const {
  if(!*this)
    return _str_const_iterator();
  return _str_const_iterator(*this, 0);
}

_str_iterator string::end() {
  if(!*this)
    return _str_iterator();
  return _str_iterator(*this, len());
}

_str_const_iterator string::cend() const {
  if(!*this)
    return _str_const_iterator();
  return _str_const_iterator(*this, len());
}

_str_iterator string::operator[](int32_t i) {
  if(!m_buf || i > m_ln || i < 0)
    return end();
  return _str_iterator(*this, i);
}

_str_const_iterator string::operator[](int32_t i) const {
  if(!m_buf || i > m_ln || i < 0)
    return cend();
  return _str_const_iterator(*this, i);
}

bool string::operator==(const string& rhs) const {
  if(!m_buf || !rhs)
    return false;
  return !strcmp(m_buf, rhs.m_buf);
}

bool string::operator!=(const string& rhs) const {
  if(!m_buf || !rhs)
    return false;
  return !!strcmp(m_buf, rhs.m_buf);
}

bool string::operator<(const string& rhs) const {
  if(!m_buf || !rhs)
    return false;
  return strcmp(m_buf, rhs.m_buf) < 0;
}

string string::operator+(const string& rhs) const {
  if(!rhs)
    return *this;
  string out;
  out += *this;
  out += rhs;
  return out;
}

string::operator bool() const {
  return m_buf;
}

std::ostream& operator<<(std::ostream& out, const scl::string& str) {
  out << str.cstr();
  return out;
}

std::ifstream& operator>>(std::ifstream& in, string& str) {
  int32_t cur = (int32_t)in.tellg();
  auto e = in.seekg(0, std::ios::end).tellg();
  int32_t l = (int32_t)e - cur;
  if(l <= 0 || l >= 0xfffffffe)
    return in;
  in.seekg(cur);
  str.reserve(l);
  str.m_ln = l;
  in.read((char*)str.cstr(), l);
  return in;
}

scl::string operator+(const scl::string& str, const char* str2) {
  return str + scl::string(str2);
}

_str_iterator::_str_iterator(string& s, int32_t i) : m_s(&s), m_i(i) {
}

bool _str_iterator::operator==(const _str_iterator& rhs) const {
  return m_s == rhs.m_s && m_i == rhs.m_i;
}

_str_iterator& _str_iterator::operator++() {
  m_i++;
  return *this;
}

_str_iterator::operator const char&() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}

const char& _str_iterator::operator*() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}

_str_iterator::operator char&() {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  return m_s->m_buf[m_i];
}

char& _str_iterator::operator*() {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  return m_s->m_buf[m_i];
}

_str_iterator& _str_iterator::operator=(char c) {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  m_s->m_buf[m_i] = c;
  return *this;
}

_str_const_iterator::_str_const_iterator(const scl::string& s, int32_t i)
    : m_s(&s), m_i(i) {
}

bool _str_const_iterator::operator==(const _str_const_iterator& rhs) const {
  return m_s == rhs.m_s && m_i == rhs.m_i;
}

_str_const_iterator& _str_const_iterator::operator++() {
  m_i++;
  return *this;
}

_str_const_iterator::operator const char&() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}

const char& _str_const_iterator::operator*() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}
} // namespace scl
