/*  sclcore.cpp
 *  Core scl utilities
 */

#include "sclcore.hpp"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  undef min
#  ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#    define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#  endif
#else
#  include <unistd.h>
#  include <math.h>
#endif

static int seed_ = 1;

static void srand_ (int seed) {
  seed_ = seed;
}

static int rand_() {
  seed_ *= (seed_ * 33 + 7) >> 2;
  return seed_;
}

static int rand_int (int min, int max) {
  return (abs (rand_()) % (max - min + 1)) + min;
}

static uint64_t fasthash64_mix (uint64_t h) {
  h ^= h >> 23;
  h *= 0x2127599bf4325c37ULL;
  h ^= h >> 47;
  return h;
}

static uint64_t fasthash64 (void const *m_buf, size_t len, uint64_t seed) {
  uint64_t const       m   = 0x880355f21e6d1965ULL;
  uint64_t const      *pos = (uint64_t const *)m_buf;
  uint64_t const      *end = pos + (len / 8);
  unsigned char const *pos2;
  uint64_t             h = seed ^ (len * m);
  uint64_t             v;

  while (pos != end) {
    v = *pos++;
    h ^= fasthash64_mix (v);
    h *= m;
  }

  pos2 = (unsigned char const *)pos;
  v    = 0;

  switch (len & 7) {
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
    h ^= fasthash64_mix (v);
    h *= m;
  }

  return fasthash64_mix (h);
}

namespace scl {
unsigned char log2i (unsigned x) {
  unsigned char r = 0;
  while (x >>= 1)
    r++;
  return r;
}

namespace internal {
// TODO: Rework GC system?
// It works fine right, now but has some drawbacks (syncronous linear time slot
// allocation), which i feel could be better

static uchar      refs[SCL_MAX_REFS] = {0};
static std::mutex g_mmut;

bool RefObj::findslot() {
  bool out = false;
  g_mmut.lock();
  for (int i = 1; i < SCL_MAX_REFS; i++) {
    if (!internal::refs[i]) {
      m_refi = i;
      incslot();
      out = true;
      break;
    }
  }
  g_mmut.unlock();
  return out;
}

void RefObj::incslot() const {
  if (m_refi)
    internal::refs[m_refi]++;
}

bool RefObj::decslot() {
  if (m_refi && !--internal::refs[m_refi]) {
    m_refi = 0;
    return true;
  }
  return false;
}

RefObj::RefObj() {
  findslot();
}

RefObj::RefObj (RefObj const &ro) {
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

bool RefObj::make_unique (bool copy) {
  // No need, already unique
  if (refs() == 1)
    return false;
  bool d = deref();
  findslot();
  if (copy)
    mutate (d);
  return true;
}

void RefObj::ref (RefObj const &ro) {
  deref();
  m_refi = ro.m_refi;
  incslot();
}
} // namespace internal

string::string() {
}

string::string (char const *str) {
  view (str);
}

string::~string() {
  if (deref() && *this) {
    delete[] m_buf;
  }
}

#ifdef _WIN32
string::string (wchar_t const *wstr) {
  m_sz  = 0;
  m_buf = NULL;
  if (wstr) {
    int n = WideCharToMultiByte (CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL) + 1;
    reserve (n);
    WideCharToMultiByte (CP_UTF8, 0, wstr, -1, m_buf, n, NULL, NULL);
  }
}
#endif

string::string (string const &rhs)
    : RefObj (rhs), m_buf (rhs.m_buf), m_ln (rhs.m_ln), m_sz (rhs.m_sz) {
}

void string::mutate (bool free) {
  m_ln       = m_buf ? m_ln : 0;
  m_sz       = m_ln;
  char *nbuf = new char[m_ln + 1];
  memset (nbuf, 0, (size_t)m_ln + 1);
  if (m_buf) {
    memcpy (nbuf, m_buf, m_ln);
    if (free)
      delete[] m_buf;
  }
  m_buf = nbuf;
}

void string::clear() {
  if (!make_unique (false) && *this) {
    delete[] m_buf;
  }
  m_buf = NULL;
  m_ln  = 0;
  m_sz  = 0;
}

string &string::claim (char const *ptr) {
  clear();
  m_buf = (char *)ptr;
  m_ln  = ptr ? (unsigned)strlen (ptr) : 0;
  m_sz  = m_ln;
  return *this;
}

string &string::view (char const *ptr) {
  // Become untracked, as we are only viewing
  deref();
  m_buf = (char *)ptr;
  m_ln  = ptr ? (unsigned)strlen (ptr) : 0;
  m_sz  = m_ln;
  return *this;
}

string &string::reserve (unsigned size) {
  char *nbuf = new char[size + 1];
  if (!nbuf)
    throw "out of memory";
  memset (nbuf, 0, (size_t)size + 1);
  if (*this)
    memcpy (nbuf, m_buf, m_sz);
  claim (nbuf);
  m_ln = len();
  m_sz = size;
  return *this;
}

char const *string::cstr() const {
  return m_buf;
}
#ifdef _WIN32
wchar_t const *string::wstr() const {
  int      wlen  = MultiByteToWideChar (CP_UTF8, 0, m_buf, -1, NULL, 0);
  int      wsize = (wlen + 1);
  wchar_t *wstr  = new wchar_t[wsize];
  memset (wstr, 0, sizeof (wchar_t) * wsize);
  MultiByteToWideChar (CP_UTF8, 0, m_buf, -1, wstr, wlen);
  return wstr;
}
#endif

long string::len() const {
  return m_ln;
}

unsigned string::size() const {
  return m_sz;
}

long string::ffi (string const &pattern) const {
  if (!*this || !pattern)
    return -1;
  char const *p   = m_buf;
  unsigned    csl = (unsigned)pattern.len();
  for (; *p; p++) {
    if (!strncmp (p, pattern.cstr(), csl))
      return (long)(p - m_buf);
  }
  return -1;
}

long string::fli (string const &pattern) const {
  if (!*this || !pattern)
    return -1;
  unsigned const l   = m_ln;
  unsigned       csl = pattern.m_ln;
  char const    *p   = m_buf + l - csl;
  for (; *p && p >= m_buf; p--) {
    if (!strncmp (p, pattern.m_buf, csl))
      return (long)(p - m_buf);
  }
  return -1;
}

bool string::endswith (string const &pattern) const {
  long p = fli (pattern);
  return p > 0 && p == m_ln - pattern.m_ln;
}

static char str_match (char const *pattern, char const *candidate, int p,
  int c) {
  if (pattern[p] == '\0') {
    return candidate[c] == '\0';
  } else if (pattern[p] == '*') {
    for (; candidate[c] != '\0'; c++) {
      if (str_match (pattern, candidate, p + 1, c))
        return 1;
    }
    return str_match (pattern, candidate, p + 1, c);
  } else if (pattern[p] != '?' && pattern[p] != candidate[c]) {
    return 0;
  } else {
    return str_match (pattern, candidate, p + 1, c + 1);
  }
}

bool string::match (string const &pattern) const {
  if (!*this || !pattern)
    return 0;
  return str_match (pattern.cstr(), cstr(), 0, 0);
}

unsigned string::hash() const {
  uint64_t h = fasthash64 (m_buf, m_ln, 1024);
  return (unsigned)(h - (h >> 32));
}

string string::substr (unsigned i, unsigned j) const {
  if (!*this || i >= m_ln)
    return "";
  j         = std::min (j, (unsigned)strlen (m_buf + i));
  char *out = new char[j + 1];
  if (!out)
    throw "out of memory";
  memset (out, 0, (size_t)j + 1);
  memcpy (out, m_buf + i, j);
  string sout;
  sout.claim (out);
  return sout;
}

string &string::replace (string const &pattern, string const &with) {
  if (!*this || !pattern || !with)
    return *this;
  char const *str = m_buf;
  string      out;
  while (1) {
    string tstr = str;
    int    p    = tstr.ffi (pattern);
    out += tstr.substr (0, p);
    if (p < 0)
      break;
    if (with)
      out += with;
    str += p + pattern.m_ln;
  }
  *this = out;
  return *this;
}

long string::ffi (char const *str, char const *pattern) {
  if (!str || !pattern)
    return -1;
  char const *p   = str;
  unsigned    csl = (unsigned)strlen (pattern);
  for (; *p; p++) {
    if (!strncmp (p, pattern, csl))
      return (long)(p - str);
  }
  return -1;
}

string string::substr (char const *str, unsigned i, unsigned j) {
  unsigned const m_ln = str ? (unsigned)strlen (str) : 0;
  if (!str || i >= m_ln)
    return "";
  j         = std::min (j, (unsigned)strlen (str + i));
  char *out = new char[j + 1];
  if (!out)
    throw "out of memory";
  memset (out, 0, (size_t)j + 1);
  memcpy (out, str + i, j);
  string sout;
  sout.claim (out);
  return sout;
}

string string::copy() const {
  return substr (0);
}

string string::rand (unsigned len) {
  static char const rchars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  string str;
  str.reserve (len);
  unsigned i;
  for (i = 0; i < len; i++) {
    str[i] = rchars[rand_int (0, sizeof (rchars) - 1)];
  }
  return str;
}

string string::vfmt (char const *fmt, va_list args) {
  va_list copy;
  va_copy (copy, args);
  int size = vsnprintf (NULL, 0, fmt, copy) + 1;
  va_end (copy);
  char *str = new char[size];
  if (!str)
    throw "out of memory";
  memset (str, 0, size);
  vsnprintf (str, size, fmt, args);
  string out;
  out.claim (str);
  return out;
}

string string::fmt (char const *fmt, ...) {
  va_list args;
  va_start (args, fmt);
  string out = vfmt (fmt, args);
  va_end (args);
  return out;
}

unsigned string::hash (string const &str) {
  return str.hash();
}

bool string::match (char const *str, char const *pattern) {
  if (!str || !pattern)
    return 0;
  return str_match (pattern, str, 0, 0);
}

internal::str_iterator string::begin() {
  if (!*this)
    return internal::str_iterator();
  return internal::str_iterator (m_buf[0], *this);
}

internal::str_iterator string::end() {
  if (!*this)
    return internal::str_iterator();
  return internal::str_iterator (m_buf[m_sz], *this);
}

internal::str_iterator string::operator[] (long i) {
  if (!m_buf || i >= (long)m_sz || i < 0)
    throw internal::str_iterator();
  return internal::str_iterator (m_buf[i], *this);
}

bool string::operator== (string const &rhs) const {
  return !strcmp (m_buf, rhs.m_buf);
}

bool string::operator!= (string const &rhs) const {
  if (!m_buf || !rhs)
    return false;
  return !!strcmp (m_buf, rhs.m_buf);
}

bool string::operator<(string const &rhs) const {
  if (!m_buf || !rhs)
    return false;
  return strcmp (m_buf, rhs.m_buf) < 0;
}

string string::operator+ (string const &rhs) const {
  if (!rhs)
    return *this;
  string out;
  out += *this;
  out += rhs;
  return out;
}

string::operator bool() const {
  return m_buf;
}

string &string::operator= (string const &rhs) {
  clear();
  ref (rhs);
  m_buf = rhs.m_buf;
  m_ln  = rhs.m_ln;
  m_sz  = rhs.m_sz;
  return *this;
}

std::ifstream &operator>> (std::ifstream &in, string &str) {
  long long cur = in.tellg();
  auto      e   = in.seekg (0, std::ios::end).tellg();
  long long l   = (long long)e - cur;
  if (l <= 0 || l >= 0xfffffffe)
    return in;
  in.seekg (cur);
  str.reserve ((unsigned)l);
  str.m_ln = (unsigned)l;
  in.read ((char *)str.cstr(), l);
  return in;
}

scl::string operator+ (scl::string const &str, char const *str2) {
  return str + scl::string (str2);
}

namespace internal {
str_iterator::str_iterator() : m_c (nullptr), m_s (nullptr) {
}

str_iterator::str_iterator (char &m_c, string &m_s) : m_c (&m_c), m_s (&m_s) {
}

bool str_iterator::operator== (str_iterator const &rhs) const {
  return m_c == rhs.m_c;
}

str_iterator &str_iterator::operator++() {
  m_c++;
  return *this;
}

str_iterator::operator char const &() const {
  if (!m_c)
    throw std::out_of_range ("Null");
  return *m_c;
}

char const &str_iterator::operator*() const {
  if (!m_c)
    throw std::out_of_range ("out of your mom");
  return *m_c;
}

char &str_iterator::operator*() {
  if (!m_c || !m_s)
    throw std::out_of_range ("out of your mom");
  m_s->make_unique();
  return *m_c;
}

str_iterator &str_iterator::operator= (char m_c) {
  if (!m_c || !m_s)
    return *this;
  long long off = m_s->m_buf - this->m_c;
  m_s->make_unique();
  this->m_c  = m_s->m_buf + off;
  *this->m_c = m_c;
  return *this;
}
} // namespace internal

#ifdef _WIN32
/* Windows sleep in 100ns units */
static BOOLEAN _nanosleep (LONGLONG ns) {
  // h_loadWinAPI();
  ns /= 100;
  /* Declarations */
  HANDLE        timer; /* Timer handle */
  LARGE_INTEGER li; /* Time defintion */
  /* Create timer */
  if (!(timer = CreateWaitableTimerExW (NULL,
          NULL,
          CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
          TIMER_ALL_ACCESS))) {
    return FALSE;
  }
  /* Set timer properties */
  li.QuadPart = -ns;
  if (!SetWaitableTimer (timer, &li, 0, NULL, NULL, FALSE)) {
    CloseHandle (timer);
    return FALSE;
  }
  /* Start & wait for timer */
  WaitForSingleObject (timer, INFINITE);
  /* Clean resources */
  CloseHandle (timer);
  /* Slept without problems */
  return TRUE;
}

static LARGE_INTEGER base_clock = {0};
#else
static double base_clock = 0.0;
#endif

void resetclock() {
#ifdef _WIN32
  QueryPerformanceCounter (&base_clock);
#else
  base_clock = clock();
#endif
}

double clock() {
#if defined(_WIN32)
  LARGE_INTEGER pc;
  LARGE_INTEGER pf;
  QueryPerformanceCounter (&pc);
  QueryPerformanceFrequency (&pf);
  return (double)(pc.QuadPart - base_clock.QuadPart) / (double)pf.QuadPart;
#elif defined(__unix__) || defined(__APPLE__)
  struct timespec ts;
  timespec_get (&ts, 1);
  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) - base_clock;
#endif
}

void waitms (double ms) {
#if defined(__unix__) || defined(__APPLE__)
  struct timespec ts = {2000, 0};
  ts.tv_sec          = ms / 1000.0;

  ts.tv_nsec = fmodf (ms, 1000) * 1000000.0;
  while (nanosleep (&ts, &ts) == -1)
    ;
#elif defined(_WIN32)
  LARGE_INTEGER li;
  QueryPerformanceCounter (&li);
  LARGE_INTEGER lf;
  QueryPerformanceFrequency (&lf);
  while (1) {
    LARGE_INTEGER li2;
    QueryPerformanceCounter (&li2);
    if ((double)(li2.QuadPart - li.QuadPart) / (double)lf.QuadPart * 1000.0 >
        ms) {
      break;
    }
    _nanosleep (1000);
  }
#endif
}

bool waitUntil (std::function<bool()> cond, double timeout, double sleep) {
  bool   infinite = timeout == -1;
  double cs       = scl::clock();
  double ce;
  bool   timedout = false;
  while (!cond() && !timedout) {
    scl::waitms (sleep);
    ce       = scl::clock();
    timedout = ((ce - cs) * 1000 < timeout && !infinite);
  }
  return !timedout;
}

Memory::Memory (Memory &&rhs) {
  if (!rhs.valid)
    return;
  std::lock (m_mut, rhs.m_mut);
  m_mut.~mutex();
  memcpy (&m_mut, &rhs.m_mut, sizeof (std::mutex));
  m_data = rhs.m_data;
  m_rp   = rhs.m_rp;
  m_wp   = rhs.m_wp;
  m_size = rhs.m_size;
  memset (&rhs, 0, sizeof (Memory));
  m_mut.unlock();
}

Memory &Memory::operator= (Memory &&rhs) {
  if (!rhs.valid)
    return *this;
  std::lock (m_mut, rhs.m_mut);
  m_mut.~mutex();
  memcpy (&m_mut, &rhs.m_mut, sizeof (std::mutex));
  m_data = rhs.m_data;
  m_rp   = rhs.m_rp;
  m_wp   = rhs.m_wp;
  m_size = rhs.m_size;
  memset (&rhs, 0, sizeof (Memory));
  m_mut.unlock();
  return *this;
}

bool Memory::reserve (size_t n, bool force) {
  if (!valid)
    return false;
  m_mut.lock();
  long long rl = (m_data + m_size) - m_wp;
  if (rl < (long long)n || force) {
    size_t    nsz  = m_size + n;
    long long roff = m_rp - m_data;
    long long woff = m_wp - m_data;

    char *buf = new char[nsz];
    if (m_data) {
      memcpy (buf, m_data, m_size);
      delete[] m_data;
    }
    m_rp   = buf + roff;
    m_wp   = buf + woff;
    m_data = buf;
    m_size = nsz;
  }
  m_mut.unlock();
  return true;
}

long long Memory::write (void const *buf, unsigned n) {
  if (!valid)
    return 0;
  reserve (n, false);
  m_mut.lock();
  memcpy (m_wp, buf, n);
  m_wp += n;
  m_mut.unlock();
  return 0;
}
} // namespace scl
