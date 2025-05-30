/*  sclcore.hpp
 *  Core scl utilities
 */

#ifndef SCL_CORE_H
#define SCL_CORE_H

#include <fstream>
#include <functional>
#include <mutex>
#include <stdarg.h>
#include <string.h>

#define SCL_MAX_REFS 4096

/**
 * @brief Main SCL namespace
 *
 */
namespace scl {
class path;
typedef unsigned char uchar;

uchar log2i (unsigned x);

/**
 * @brief SCL internal namespace
 *
 */
namespace internal {
class RefObj {
 private:
  int m_refi = 0;

  bool findslot();
  void incslot() const;
  bool decslot();

 public:
  RefObj();
  RefObj (const RefObj &);
  virtual ~RefObj() = 0;

 protected:
  /**
   * @brief Deep copy implemented by derived classes, called by RefObj.
   * @note The `free` param can be ignored by most implementations, unless they
   * use unmanaged memory (example: an scl::string object viewing a string
   * buffer).
   *
   * @param free Whether or not this object is managed, and its resources
   * should be freed during the copy.
   */
  virtual void mutate (bool free) = 0;

  /**
   * @brief Decrements the number of references for this object.
   *
   * @return <b>true</b> if no references remain.
   * @return <b>false</b> if otherwise.
   */
  bool deref();

  /**
   * @brief Makes this object unique.
   * @note Calles the derived class' implmentation of
   * RefObj::mutate() if applicable.
   *
   * @param copy Whether or not this object should be deep copied if it
   * is not unique already.
   * @return <b>true</b> if this object was not unique before.
   * @return <b>false</b> if otherwise.
   */
  bool make_unique (bool copy = true);

  /**
   * @brief Makes this object reference another. Behaves identically to the
   * RefObj copy constructor.
   * @note This does not handle any managed memory of the derived class, which
   * must be handled manually before using this method.
   */
  void ref (const RefObj &);

  bool operator== (const RefObj &rhs) const;

 public:
  /**
   * @brief Returns the number of references of this object.
   *
   * @return <b>int</b> Number of references. Returns 0 if this object has no
   * references.
   */
  int refs() const;
};

class str_iterator;
} // namespace internal

class string : public internal::RefObj {
 private:
  friend class internal::str_iterator;

  char    *m_buf = nullptr;
  unsigned m_ln  = 0;
  unsigned m_sz  = 0;

  void mutate (bool free) override;

 public:
  string();
  string (const char *);
  string (char);
#ifdef _WIN32
  string (const wchar_t *);
#endif
  string (const string &);
  ~string() override;

  /**
   * @brief Clears this strings memory.
   *
   */
  void clear();

  /**
   * @brief Takes ownership of `ptr`.
   * @note The given pointer will be treated as if scl::string created it, so
   * ensure no other structures are managing it.
   *
   * @param ptr Pointer to take ownership of.
   */
  string &claim (const char *ptr);

  /**
   * @brief Turns this string into a readonly layer above a given string buffer.
   *
   * @param ptr Pointer to string buffer to view.
   */
  string &view (const char *ptr);

  /**
   * @brief Increases the capacity of this string object, copying the original
   * contents. Can be used to shrink its capacity.
   *
   * @param size Size in bytes to set the capacity to.
   */
  string &reserve (unsigned size);

  /**
   * @brief Returns the managed string buffer of this string object. Does not
   * need to be freed.
   * @note If the managed buffer is free'd, the pointer returned by this
   * function will become invalid.
   *
   * @return Pointer to this string's managed buffer. NULL if
   * this string has no contents.
   */
  const char *cstr() const;
#ifdef _WIN32
  /**
   * @brief Returns a wchar_t version of this string.
   * @note The buffer returned by this function is NOT managed, and must be
   * freed manually.
   *
   * @return Pointer to a wchar_t string buffer. NULL if
   * the function failed.
   */
  const wchar_t *wstr() const;
#endif

  /**
   * @brief Attempts to convert as much of this string into an integer as
   * possible. Automatically detects hexedecimal literals.
   *
   * @return  An integer representation of this string.
   */
  long long toInt() const;

  /**
   * @brief Returns the length of this string, excluding null terminator.
   *
   * @return Length in bytes.
   */
  long len() const;

  /**
   * @brief Returns the capacity of this string.
   *
   * @return Capacity in bytes.
   */
  unsigned size() const;

  /**
   * @brief Finds the first instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the first instance found. -1 if no instance is
   * found.
   */
  long ffi (const string &pattern) const;

  /**
   * @brief Finds the last instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the last instance found. -1 if no instance is
   * found.
   */
  long fli (const string &pattern) const;

  /**
   * @brief Returns whether or not this string ends with a pattern.
   *
   * @param pattern A string pattern to check with (no wildcard supported).
   * @return true if this string ends with `pattern`, false if otherwise.
   */
  bool endswith (const string &pattern) const;

  /**
   * @brief Reterns whether or not this string constains a pattern.
   *
   * @param pattern A string pattern to search for (wildcard and ?
   * supported).
   * @return true if this string contains `pattern` at least once, false if
   * otherwise.
   */
  bool match (const string &pattern) const;

  /**
   * @brief Returns a hash of this string.
   *
   */
  unsigned hash() const;

  /**
   * @brief Returns a substring of this string.
   *
   * @param i The start index of the substring.
   * @param j The length of the substring.
   */
  string substr (unsigned i, unsigned j = -1) const;

  /**
   * @brief Returns a unique copy of this string.
   */
  string copy() const;

  /**
   * @brief Replaces any instance of a pattern in this string with a
   * replacement.
   *
   * @param pattern A string pattern to replace (no wildcard supported).
   * @param with Replacement string.
   */
  string &replace (const string &pattern, const string &with);

  /**
   * @brief Replaces all lowercase ascii characters with their uppercase
   * counterparts.
   */
  string &toUpper();

  static long   ffi (const char *str, const char *pattern);
  static string substr (const char *str, unsigned i, unsigned j);

  /**
   * @brief Returns a randomized string of a specified length.
   *
   * @param len Length in characters of the randomized string.
   */
  static string rand (unsigned len);
  static string vfmt (const char *fmt, va_list args);

  /**
   * @brief Returns a formatted string.
   *
   * @param fmt A C-Style string format.
   * @param ... Formatting arguments.
   */
  static string   fmt (const char *fmt, ...);
  static unsigned hash (const string &str);
  static bool     match (const char *str, const char *pattern);

  internal::str_iterator begin();
  internal::str_iterator end();

  internal::str_iterator operator[] (long);

  bool operator== (const string &) const;
  bool operator!= (const string &) const;
  bool operator< (const string &) const;

  string operator+ (const string &) const;

  template <int step = 1>
  string &operator+= (char rhs) {
    char s[2] = {rhs, '\0'};
    return (*this).operator+= <step> (string().view (s));
  }

  template <int step = 1>
  string &operator+= (const string &rhs) {
    if (!rhs)
      return *this;
    make_unique();
    if (!*this || m_ln + rhs.m_ln >= m_sz - 1) {
      const unsigned m   = ((m_ln + rhs.m_ln + (step - 1)) / step);
      unsigned       req = m * step;
      reserve (req);
    }
    memcpy (m_buf + m_ln, rhs.m_buf, (size_t)rhs.m_ln + 1);
    m_ln = m_ln + rhs.m_ln;
    return *this;
  }

  operator bool() const;

  string &operator= (const string &);

  friend std::ifstream &operator>> (std::ifstream &in, string &str);
};

scl::string operator+ (const scl::string &str, const char *str2);

/**
 * @brief Resets the output of scl::clock(), making current time epoch.
 *
 */
void resetclock();

/**
 * @note You can use scl::resetclock() to control this function's epoch.
 *
 * @return   Seconds since epoch.
 */
double clock();

/**
 * @brief Makes this thread sleep for a given amount of milliseconds.
 *
 * @param ms  Number of milliseconds to sleep for.
 */
void waitms (double ms);

bool waitUntil (std::function<bool()> cond, double timeout = -1,
  double sleep = 0.001);

enum class StreamPos {
  start   = SEEK_SET,
  end     = SEEK_END,
  current = SEEK_CUR,
};

class stream {
 protected:
  char  *m_data   = nullptr;
  char  *m_rp     = nullptr;
  char  *m_wp     = nullptr;
  FILE  *m_stream = nullptr;
  size_t m_size   = 0;

  long long bounds (const char *p, size_t n) const;

  bool swrite (const void *buf, size_t n);

 public:
  stream() = default;
  ~stream();

  bool open (const scl::path &path, bool binary = false);
  void flush();

  long long seek (StreamPos pos, long long off);
  long long tell() const;
  long long read (void *buf, size_t n);

  bool reserve (size_t n, bool force = false);
  bool write (const void *buf, size_t n);
  bool write (const scl::string &str);

  void close();

  stream &operator<< (const scl::string &str);
};

namespace internal {
class str_iterator {
  string  *m_s = nullptr;
  unsigned m_i = -1;

 public:
  str_iterator() = default;
  str_iterator (string &m_s, unsigned i);

  bool          operator== (const str_iterator &rhs) const;
  str_iterator &operator++();

  /* Read */
  operator const char &() const;
  const char &operator*() const;

  /* Write */
  operator char &();
  char         &operator*();
  str_iterator &operator= (char c);
};
} // namespace internal

} // namespace scl
#endif