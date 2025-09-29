/*  sclcore.cpp
 *  Core scl utilities
 */

#include <mutex>
#include "sclcore.hpp"
#include "sclpath.hpp"
#include "sclpack.hpp"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  undef min
#  undef max
#  ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#    define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#  endif
#else
#  include <unistd.h>
#  include <math.h>
#endif

static int  seed_ = 1;

static void srand_(int seed) {
  seed_ = seed;
}

static int rand_() {
  seed_ *= (seed_ * 33 + 7) >> 2;
  return seed_;
}

static int rand_int(int min, int max) {
  return (abs(rand_()) % (max - min + 1)) + min;
}

static uint64_t fasthash64_mix(uint64_t h) {
  h ^= h >> 23;
  h *= 0x2127599bf4325c37ULL;
  h ^= h >> 47;
  return h;
}

static uint64_t fasthash64(const void *m_buf, size_t len, uint64_t seed) {
  const uint64_t       m   = 0x880355f21e6d1965ULL;
  const uint64_t      *pos = (const uint64_t *)m_buf;
  const uint64_t      *end = pos + (len / 8);
  const unsigned char *pos2;
  uint64_t             h = seed ^ (len * m);
  uint64_t             v;

  while(pos != end) {
    v = *pos++;
    h ^= fasthash64_mix(v);
    h *= m;
  }

  pos2 = (const unsigned char *)pos;
  v    = 0;

  switch(len & 7) {
  case 7:
    v ^= (uint64_t)pos2[6] << 48;
  case 6:
    v ^= (uint64_t)pos2[5] << 40;
  case 5:
    v ^= (uint64_t)pos2[4] << 32;
  case 4:
    v ^= (uint64_t)pos2[3] << 24;
  case 3:
    v ^= (uint64_t)pos2[2] << 16;
  case 2:
    v ^= (uint64_t)pos2[1] << 8;
  case 1:
    v ^= (uint64_t)pos2[0];
    h ^= fasthash64_mix(v);
    h *= m;
  }

  return fasthash64_mix(h);
}

namespace scl {
unsigned char log2i(unsigned x) {
  unsigned char r = 0;
  while(x >>= 1)
    r++;
  return r;
}

namespace internal {
// TODO: Rework GC system?
// It works fine right, now but has some drawbacks (syncronous linear time slot
// allocation), which i feel could be better

static uchar       refs[SCL_MAX_REFS] = {0};
static std::mutex *g_mmut             = nullptr;

bool               RefObj::findslot() {
  bool out = false;
  // Shrug
  if(!g_mmut)
    return false;
  g_mmut->lock();
  for(int i = 1; i < SCL_MAX_REFS; i++) {
    if(!internal::refs[i]) {
      m_refi = i;
      if(m_refi)
        internal::refs[m_refi]++;
      out = true;
      break;
    }
  }
  g_mmut->unlock();
  return out;
}

void RefObj::incslot() const {
  if(!g_mmut)
    return;
  g_mmut->lock();
  if(m_refi)
    internal::refs[m_refi]++;
  g_mmut->unlock();
}

bool RefObj::decslot() {
  if(!g_mmut)
    return false;
  g_mmut->lock();
  bool out = false;
  if(m_refi && !--internal::refs[m_refi]) {
    m_refi = 0;
    out    = true;
  }
  g_mmut->unlock();
  return out;
}

RefObj::RefObj() {
  findslot();
}

RefObj::RefObj(const RefObj &ro) {
  m_refi = ro.m_refi;
  incslot();
}

RefObj::~RefObj() {
}

int RefObj::refs() const {
  return m_refi ? internal::refs[m_refi] : 0;
}

bool RefObj::deref() {
  return decslot();
}

bool RefObj::make_unique(bool copy) {
  // No need, already unique
  if(refs() == 1)
    return false;
  bool d = deref();
  findslot();
  if(copy)
    mutate(d);
  return true;
}

void RefObj::ref(const RefObj &ro) {
  deref();
  m_refi = ro.m_refi;
  incslot();
}

bool RefObj::operator==(const RefObj &rhs) const {
  return m_refi && m_refi == rhs.m_refi;
}
} // namespace internal

string::string() {
}

string::string(const char *str) {
  view(str);
}

string::~string() {
  if(deref() && *this) {
    delete[] m_buf;
  }
}

#ifdef _WIN32
string::string(const wchar_t *wstr) {
  m_sz  = 0;
  m_buf = nullptr;
  if(wstr) {
    int n =
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr) +
      1;
    reserve(n);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, m_buf, n, nullptr, nullptr);
  }
}
#endif

string::string(const string &rhs)
    : RefObj(rhs), m_buf(rhs.m_buf), m_ln(rhs.m_ln), m_sz(rhs.m_sz) {
}

string &string::operator=(const string &rhs) {
  if(this->internal::RefObj::operator==(rhs))
    return *this;
  clear();
  ref(rhs);
  m_buf = rhs.m_buf;
  m_ln  = rhs.m_ln;
  m_sz  = rhs.m_sz;
  return *this;
}

void string::mutate(bool free) {
  m_ln = m_buf ? m_ln : 0;
  m_sz = m_buf ? m_sz : 0;
  if(m_ln) {
    char *nbuf = new char[m_sz + 1];
    memset(nbuf, 0, (size_t)m_sz + 1);
    if(m_buf) {
      memcpy(nbuf, m_buf, m_sz);
      if(free)
        delete[] m_buf;
    }
    m_buf = nbuf;
  } else {
    m_buf = nullptr;
    m_ln  = 0;
    m_sz  = 0;
  }
}

void string::clear() {
  if(!make_unique(false) && *this) {
    delete[] m_buf;
  }
  m_buf = nullptr;
  m_ln  = 0;
  m_sz  = 0;
}

string &string::claim(const char *ptr) {
  clear();
  m_buf = (char *)ptr;
  m_ln  = ptr ? (unsigned)strlen(ptr) : 0;
  m_sz  = m_ln;
  return *this;
}

string &string::view(const char *ptr) {
  // Become untracked, as we are only viewing
  deref();
  m_buf = (char *)ptr;
  m_ln  = ptr ? (unsigned)strlen(ptr) : 0;
  m_sz  = m_ln;
  return *this;
}

string &string::reserve(unsigned size) {
  char *nbuf = new char[size + 1];
  if(!nbuf)
    throw "out of stream";
  memset(nbuf, 0, (size_t)size + 1);
  if(*this)
    memcpy(nbuf, m_buf, m_sz);
  claim(nbuf);
  m_ln = (unsigned)len();
  m_sz = size;
  return *this;
}

const char *string::cstr() const {
  return m_buf;
}
#ifdef _WIN32
const wchar_t *string::wstr() const {
  if(!m_buf)
    return nullptr;
  int      wlen  = MultiByteToWideChar(CP_UTF8, 0, m_buf, -1, nullptr, 0);
  int      wsize = (wlen + 1);
  wchar_t *wstr  = new wchar_t[wsize];
  memset(wstr, 0, sizeof(wchar_t) * wsize);
  MultiByteToWideChar(CP_UTF8, 0, m_buf, -1, wstr, wlen);
  return wstr;
}
#endif

#define _isHex(c) ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
#define _fromHex(c)                        \
  ((c >= 'A' && c <= 'F') ? (c - 'A' + 10) \
                          : ((c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0))

long long string::toInt() const {
  bool hex = false;
  for(unsigned i = 0; i < len(); i++) {
    char c = m_buf[i];
    if(_isHex(c)) {
      hex = true;
      break;
    }
  }
  long long o = 0;
  for(unsigned i = 0; i < len(); i++) {
    char c = m_buf[i];
    int  m = 0;
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

unsigned string::len() const {
  return m_ln;
}

unsigned string::size() const {
  return m_sz;
}

long long string::ffi(const string &pattern) const {
  if(!*this || !pattern)
    return -1;
  const char *p   = m_buf;
  unsigned    csl = (unsigned)pattern.len();
  for(; *p; p++) {
    if(!strncmp(p, pattern.cstr(), csl))
      return (long long)(p - m_buf);
  }
  return -1;
}

long long string::fli(const string &pattern) const {
  if(!*this || !pattern)
    return -1;
  const unsigned l   = m_ln;
  unsigned       csl = pattern.m_ln;
  const char    *p   = m_buf + l - csl;
  for(; *p && p >= m_buf; p--) {
    if(!strncmp(p, pattern.m_buf, csl))
      return (long long)(p - m_buf);
  }
  return -1;
}

bool string::endswith(const string &pattern) const {
  long long p = fli(pattern);
  return p > 0 && p == m_ln - pattern.m_ln;
}

static char str_match(const char *pattern, const char *candidate, int p,
  int c) {
  if(pattern[p] == '\0') {
    return candidate[c] == '\0';
  } else if(pattern[p] == '*') {
    for(; candidate[c] != '\0'; c++) {
      if(str_match(pattern, candidate, p + 1, c))
        return 1;
    }
    return str_match(pattern, candidate, p + 1, c);
  } else if(pattern[p] != '?' && pattern[p] != candidate[c]) {
    return 0;
  } else {
    return str_match(pattern, candidate, p + 1, c + 1);
  }
}

bool string::match(const string &pattern) const {
  if(!*this || !pattern)
    return 0;
  return str_match(pattern.cstr(), cstr(), 0, 0);
}

unsigned string::hash() const {
  uint64_t h = fasthash64(m_buf, m_ln, 1024);
  return (unsigned)(h - (h >> 32));
}

string string::substr(unsigned i, unsigned j) const {
  if(!*this || i >= m_ln)
    return "";
  j         = std::min(j, (unsigned)strlen(m_buf + i));
  char *out = new char[j + 1];
  if(!out)
    throw "out of stream";
  memset(out, 0, (size_t)j + 1);
  memcpy(out, m_buf + i, j);
  string sout;
  sout.claim(out);
  return sout;
}

string &string::replace(const string &pattern, const string &with) {
  if(!*this || !pattern || !with)
    return *this;
  const char *str = m_buf;
  string      out;
  while(1) {
    string    tstr = str;
    long long p    = tstr.ffi(pattern);
    out += tstr.substr(0, (unsigned)p);
    if(p < 0)
      break;
    if(with)
      out += with;
    str += p + pattern.m_ln;
  }
  *this = out;
  return *this;
}

string &string::toUpper() {
  for(auto &c : *this) {
    if(c >= 'a' && c <= 'z') {
      c -= 32;
    }
  }
  return *this;
}

long long string::ffi(const char *str, const char *pattern) {
  if(!str || !pattern)
    return -1;
  const char *p   = str;
  unsigned    csl = (unsigned)strlen(pattern);
  for(; *p; p++) {
    if(!strncmp(p, pattern, csl))
      return (long long)(p - str);
  }
  return -1;
}

string string::substr(const char *str, unsigned i, unsigned j) {
  const unsigned m_ln = str ? (unsigned)strlen(str) : 0;
  if(!str || i >= m_ln)
    return "";
  j         = std::min(j, (unsigned)strlen(str + i));
  char *out = new char[j + 1];
  if(!out)
    throw "out of stream";
  memset(out, 0, (size_t)j + 1);
  memcpy(out, str + i, j);
  string sout;
  sout.claim(out);
  return sout;
}

string string::copy() const {
  return substr(0);
}

string string::rand(unsigned len) {
  static const char rchars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  string str;
  str.reserve(len);
  unsigned i;
  for(i = 0; i < len; i++) {
    str[i] = rchars[rand_int(0, sizeof(rchars) - 1)];
  }
  return str;
}

string string::vfmt(const char *fmt, va_list args) {
  va_list copy;
  va_copy(copy, args);
  int size = vsnprintf(nullptr, 0, fmt, copy) + 1;
  va_end(copy);
  char *str = new char[size];
  if(!str)
    throw "out of stream";
  memset(str, 0, size);
  vsnprintf(str, size, fmt, args);
  string out;
  out.claim(str);
  return out;
}

string string::fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  string out = vfmt(fmt, args);
  va_end(args);
  return out;
}

unsigned string::hash(const string &str) {
  return str.hash();
}

bool string::match(const char *str, const char *pattern) {
  if(!str || !pattern)
    return 0;
  return str_match(pattern, str, 0, 0);
}

internal::str_iterator string::begin() {
  if(!*this)
    return internal::str_iterator();
  return internal::str_iterator(*this, 0);
}

internal::str_iterator string::end() {
  if(!*this)
    return internal::str_iterator();
  return internal::str_iterator(*this, len());
}

internal::str_iterator string::operator[](long long i) {
  if(!m_buf || (unsigned)i > m_ln || i < 0)
    return end();
  return internal::str_iterator(*this, (unsigned)i);
}

bool string::operator==(const string &rhs) const {
  return !strcmp(m_buf, rhs.m_buf);
}

bool string::operator!=(const string &rhs) const {
  if(!m_buf || !rhs)
    return false;
  return !!strcmp(m_buf, rhs.m_buf);
}

bool string::operator<(const string &rhs) const {
  if(!m_buf || !rhs)
    return false;
  return strcmp(m_buf, rhs.m_buf) < 0;
}

string string::operator+(const string &rhs) const {
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

std::ifstream &operator>>(std::ifstream &in, string &str) {
  long long cur = in.tellg();
  auto      e   = in.seekg(0, std::ios::end).tellg();
  long long l   = (long long)e - cur;
  if(l <= 0 || l >= 0xfffffffe)
    return in;
  in.seekg(cur);
  str.reserve((unsigned)l);
  str.m_ln = (unsigned)l;
  in.read((char *)str.cstr(), l);
  return in;
}

scl::string operator+(const scl::string &str, const char *str2) {
  return str + scl::string(str2);
}

namespace internal {

str_iterator::str_iterator(string &s, unsigned i) : m_s(&s), m_i(i) {
}

bool str_iterator::operator==(const str_iterator &rhs) const {
  return m_s == rhs.m_s && m_i == rhs.m_i;
}

str_iterator &str_iterator::operator++() {
  m_i++;
  return *this;
}

str_iterator::operator const char &() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}

const char &str_iterator::operator*() const {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  return m_s->m_buf[m_i];
}

str_iterator::operator char &() {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  return m_s->m_buf[m_i];
}

char &str_iterator::operator*() {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  return m_s->m_buf[m_i];
}

str_iterator &str_iterator::operator=(char c) {
  if(!m_s || m_i > m_s->size())
    throw std::out_of_range("");
  m_s->make_unique();
  m_s->m_buf[m_i] = c;
  return *this;
}
} // namespace internal

#ifdef _WIN32
/* Windows sleep in 100ns units */
static BOOLEAN _nanosleep(LONGLONG ns) {
  // h_loadWinAPI();
  ns /= 100;
  /* Declarations */
  HANDLE        timer; /* Timer handle */
  LARGE_INTEGER li;    /* Time defintion */
  /* Create timer */
  if(!(timer = CreateWaitableTimerExW(nullptr, nullptr,
         CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))) {
    return FALSE;
  }
  /* Set timer properties */
  li.QuadPart = -ns;
  if(!SetWaitableTimer(timer, &li, 0, nullptr, nullptr, FALSE)) {
    CloseHandle(timer);
    return FALSE;
  }
  /* Start & wait for timer */
  WaitForSingleObject(timer, INFINITE);
  /* Clean resources */
  CloseHandle(timer);
  /* Slept without problems */
  return TRUE;
}

static LARGE_INTEGER base_clock = {0};
#else
static double base_clock = 0.0;
#endif

void resetclock() {
#ifdef _WIN32
  QueryPerformanceCounter(&base_clock);
#else
  base_clock = clock();
#endif
}

double clock() {
#if defined(_WIN32)
  LARGE_INTEGER pc;
  LARGE_INTEGER pf;
  QueryPerformanceCounter(&pc);
  QueryPerformanceFrequency(&pf);
  return (double)(pc.QuadPart - base_clock.QuadPart) / (double)pf.QuadPart;
#elif defined(__unix__) || defined(__APPLE__)
  struct timespec ts;
  timespec_get(&ts, 1);
  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) - base_clock;
#endif
}

void waitms(double ms) {
#if defined(__unix__) || defined(__APPLE__)
  struct timespec ts = {2000, 0};
  ts.tv_sec          = ms / 1000.0;

  ts.tv_nsec         = fmodf(ms, 1000) * 1000000.0;
  while(nanosleep(&ts, &ts) == -1)
    ;
#elif defined(_WIN32)
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  LARGE_INTEGER lf;
  QueryPerformanceFrequency(&lf);
  while(1) {
    LARGE_INTEGER li2;
    QueryPerformanceCounter(&li2);
    if((double)(li2.QuadPart - li.QuadPart) / (double)lf.QuadPart * 1000.0 >
       ms) {
      break;
    }
    _nanosleep(1000);
  }
#endif
}

bool waitUntil(std::function<bool()> cond, double timeout, double sleepms) {
  bool   infinite = timeout == -1;
  double cs       = scl::clock();
  double ce;
  bool   timedout = false;
  while(!cond() && !timedout) {
    scl::waitms(sleepms);
    ce       = scl::clock();
    timedout = ((ce - cs) * 1000 < timeout && !infinite);
  }
  return !timedout;
}

stream::stream(stream &&rhs) {
  m_stream       = rhs.m_stream;
  m_data         = rhs.m_data;
  m_fp           = rhs.m_fp;
  m_size         = rhs.m_size;
  m_ronly        = rhs.m_ronly;
  m_wonly        = rhs.m_wonly;
  m_modified     = rhs.m_modified;

  rhs.m_stream   = 0;
  rhs.m_data     = 0;
  rhs.m_fp       = 0;
  rhs.m_size     = 0;
  rhs.m_ronly    = 0;
  rhs.m_wonly    = 0;
  rhs.m_modified = 0;
}

stream &stream::operator=(stream &&rhs) {
  m_stream       = rhs.m_stream;
  m_data         = rhs.m_data;
  m_fp           = rhs.m_fp;
  m_size         = rhs.m_size;
  m_ronly        = rhs.m_ronly;
  m_wonly        = rhs.m_wonly;
  m_modified     = rhs.m_modified;

  rhs.m_stream   = 0;
  rhs.m_data     = 0;
  rhs.m_fp       = 0;
  rhs.m_size     = 0;
  rhs.m_ronly    = 0;
  rhs.m_wonly    = 0;
  rhs.m_modified = 0;
  return *this;
}

stream::~stream() {
  flush();
  if(m_stream) {
    fclose(m_stream);
    m_stream = nullptr;
  }
  if(m_data) {
    delete[] m_data;
  }
}

long long stream::bounds(const char *p, size_t n) const {
  n = std::min(n, m_size);
  if(p >= m_data + m_size)
    return 0;
  size_t s = (m_size - n), o = (m_data - p);
  if(n < s + o)
    return 0;
  return n - s - o;
}

long long stream::read_internal(void *buf, size_t n) {
  if(m_stream) {
    if(n == (size_t)-1) {
      auto o = tell();
      seek(StreamPos::end, 0);
      auto l = tell() - o;
      seek(StreamPos::start, o);
      n = std::min(n, (size_t)l);
    }
    if(feof(m_stream))
      return 0;
    auto r = fread(buf, 1, n, m_stream);
    // Suggested by gnu.org, cause r+/w+ modes are weird
    fflush(m_stream);
    return r;
  }
  size_t r = bounds(m_fp, n);
  if(!r)
    return 0;
  memcpy(buf, m_fp, r);
  m_fp += r;
  return r;
}

bool stream::write_internal(const void *buf, size_t n, size_t align) {
  if(!buf)
    return false;
  if(m_stream) {
    bool r = !n || fwrite(buf, n, 1, m_stream);
    // Suggested by gnu.org, cause r+/w+ modes are weird
    fflush(m_stream);
    return r;
  }
  long long res = ((tell() + n + (align - 1)) / align) * align - m_size;
  if(res > 0) {
    if(!reserve(res))
      return false;
  }
  memcpy(m_fp, buf, n);
  m_fp += n;
  m_modified = true;
  return true;
}

bool stream::is_open() const {
  return m_stream;
}

bool stream::is_modified() const {
  return m_modified;
}

long long stream::tell() const {
  if(m_stream) {
    return ftell(m_stream);
  }
  return m_fp - m_data;
}

void stream::reset_modified() {
  m_modified = false;
}

bool stream::openMode(const scl::path &path, const scl::string &mode) {
  if(m_stream)
    return false;
  m_ronly = mode == "r" || mode == "rb" || m_ronly;
  m_wonly = mode == "w" || mode == "wb" || m_wonly;
#ifdef _MSC_VER
#  pragma warning(disable : 4996)
#endif
  m_stream = fopen(path.cstr(), mode.cstr());
  if(m_stream)
    seek(StreamPos::start, 0);
  if(m_data && m_stream)
    flush();
  return m_stream;
}

bool stream::open(const scl::path &path, bool trunc, bool binary) {
  // w+ creates the file, and truncates, and allows fseek to read and write.
  // r+ doesnt truncate the file, and allows fseek to read and write.
  const char *mode = (trunc || !path.exists()) ? (binary ? "wb+" : "w+")
                                               : (binary ? "rb+" : "r+");
  return openMode(path, mode);
}

void stream::flush() {
  if(m_data && m_stream) {
    write_internal(m_data, m_size, 1);
    delete[] m_data;
    m_data = nullptr;
    m_fp   = nullptr;
    m_size = 0;
  }
  if(m_stream)
    fflush(m_stream);
}

long long stream::seek(StreamPos pos, long long off) {
  if(m_stream) {
    fseek(m_stream, (long)off, (int)pos);
    fflush(m_stream);
    return ftell(m_stream);
  }
  if(pos == StreamPos::start)
    m_fp = m_data;
  else if(pos == StreamPos::end)
    m_fp = m_data + m_size;
  m_fp += off;
  m_fp = std::max(m_fp, m_data);
  return m_fp - m_data;
}

long long stream::read(void *buf, size_t n) {
  if(m_wonly)
    return 0;
  return read_internal(buf, n);
}

bool stream::reserve(size_t n, bool force) {
  // Ignore file mode
  if(m_stream)
    return true;
  // Remaining length
  long long rl = (m_data + m_size) - m_fp;
  if(rl < (long long)n || force) {
    size_t    nsz  = m_size + n;
    long long foff = m_fp - m_data;

    char     *buf  = new char[nsz];
    if(!buf)
      return false;
    if(m_data) {
      memcpy(buf, m_data, m_size);
      delete[] m_data;
    }
    memset(buf + m_size, 0, nsz - m_size);
    m_fp   = buf + foff;
    m_data = buf;
    m_size = nsz;
  }
  return true;
}

bool stream::write(const void *buf, size_t n, size_t align, bool flush) {
  if(m_ronly)
    return false;
  // Just ignore flush?
  return write_internal(buf, n, align);
}

bool stream::write(const scl::string &str, size_t align, bool flush) {
  return write(str.cstr(), str.len(), align, flush);
}

bool stream::write(stream &src, size_t max) {
  char   buf[SCL_STREAM_BUF];
  size_t total = 0;
  bool   r     = false;
  do {
    if(total >= max)
      break;
    size_t read      = src.read(buf, SCL_STREAM_BUF);
    size_t avail     = max - total;
    size_t readBytes = avail < read ? avail : read;
    total += read;
    if(readBytes)
      // Write. Flush if the streaming buffer isnt full (usually end of
      // streaming).
      r = write(buf, readBytes, 1, readBytes < SCL_STREAM_BUF);
    if(!readBytes || !r)
      break;
  } while(1);
  return r;
}

void stream::close() {
  flush();
  if(m_stream) {
    fclose(m_stream);
    m_stream = nullptr;
  }
  if(m_data)
    delete[] m_data;
  // Reset members
  scl::stream();
}

const void *stream::data() {
  return m_data;
}

void *stream::release() {
  void *ptr = nullptr;
  if(m_data) {
    ptr = m_data;
    // Reset members
    scl::stream();
  }
  return ptr;
}

stream &stream::operator<<(const scl::string &str) {
  write(str);
  return *this;
}

stream &stream::operator>>(scl::string &str) {
  auto      off = tell();
  long long end = seek(StreamPos::end, 0);
  seek(StreamPos::start, off);
  if(end >= UINT_MAX)
    return *this;
  char *buf = new char[SCL_STREAM_BUF];
  str.reserve((unsigned)end);
  for(;;) {
    auto readBytes = read(buf, SCL_STREAM_BUF - 1);
    if(!readBytes)
      break;
    buf[readBytes] = 0;
    str.operator+= <256>(buf);
  }
  delete[] buf;
  return *this;
}

bool init() {
  internal::g_mmut = new std::mutex();
  bool pack        = pack::packInit();
  return pack;
}

void terminate() {
  pack::packTerminate();
  delete internal::g_mmut;
  memset(internal::refs, 0, sizeof(internal::refs));
  internal::g_mmut = nullptr;
}
} // namespace scl
