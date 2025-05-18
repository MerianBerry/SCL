/*  sclcore.hpp
 *  Core scl utilities
 */

#ifndef SCL_CORE_H
#define SCL_CORE_H

#include <fstream>
#include <mutex>
#include <stdarg.h>
#include <string.h>

#define SCL_MAX_REFS 4096

/**
 * @brief Main SCL namespace
 *
 */
namespace scl {
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
  RefObj (RefObj const &);
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
   * @brief Returns the number of references of this object.
   *
   * @return <b>int</b> Number of references. Returns 0 if this object has no
   * references.
   */
  int refs() const;

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
  void ref (RefObj const &);
};

class str_iterator;
} // namespace internal

class string : internal::RefObj {
 private:
  friend class internal::str_iterator;

  char    *m_buf = nullptr;
  unsigned m_ln  = 0;
  unsigned m_sz  = 0;

  void mutate (bool free) override;

 public:
  string();
  string (char const *);
  string (char);
#ifdef _WIN32
  string (wchar_t const *);
#endif
  string (string const &);
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
  string &claim (char const *ptr);

  /**
   * @brief Turns this string into a readonly layer above a given string buffer.
   *
   * @param ptr Pointer to string buffer to view.
   */
  string &view (char const *ptr);

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
  char const *cstr() const;
#ifdef _WIN32
  /**
   * @brief Returns a wchar_t version of this string.
   * @note The buffer returned by this function is NOT managed, and must be
   * freed manually.
   *
   * @return Pointer to a wchar_t string buffer. NULL if
   * the function failed.
   */
  wchar_t const *wstr() const;
#endif

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
  long ffi (string const &pattern) const;

  /**
   * @brief Finds the last instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the last instance found. -1 if no instance is
   * found.
   */
  long fli (string const &pattern) const;

  /**
   * @brief Returns whether or not this string ends with a pattern.
   *
   * @param pattern A string pattern to check with (no wildcard supported).
   * @return true if this string ends with `pattern`, false if otherwise.
   */
  bool endswith (string const &pattern) const;

  /**
   * @brief Reterns whether or not this string constains a pattern.
   *
   * @param pattern A string pattern to search for (wildcard and ?
   * supported).
   * @return true if this string contains `pattern` at least once, false if
   * otherwise.
   */
  bool match (string const &pattern) const;

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
  string &replace (string const &pattern, string const &with);

  static long   ffi (char const *str, char const *pattern);
  static string substr (char const *str, unsigned i, unsigned j);

  /**
   * @brief Returns a randomized string of a specified length.
   *
   * @param len Length in characters of the randomized string.
   */
  static string rand (unsigned len);
  static string vfmt (char const *fmt, va_list args);

  /**
   * @brief Returns a formatted string.
   *
   * @param fmt A C-Style string format.
   * @param ... Formatting arguments.
   */
  static string   fmt (char const *fmt, ...);
  static unsigned hash (string const &str);
  static bool     match (char const *str, char const *pattern);

  internal::str_iterator begin();
  internal::str_iterator end();

  internal::str_iterator operator[] (long);

  bool operator== (string const &) const;
  bool operator!= (string const &) const;
  bool operator< (string const &) const;

  string operator+ (string const &) const;

  template <int step = 1>
  string &operator+= (char rhs) {
    char s[2] = {rhs, '\0'};
    return (*this).operator+= <step> (string().view (s));
  }

  template <int step = 1>
  string &operator+= (string const &rhs) {
    if (!rhs)
      return *this;
    make_unique();
    if (!*this || m_ln + rhs.m_ln >= m_sz - 1) {
      unsigned const m   = ((m_ln + rhs.m_ln) / step + 2);
      unsigned       req = m * step;
      reserve (req);
    }
    memcpy (m_buf + m_ln, rhs.m_buf, (size_t)rhs.m_ln + 1);
    m_ln = m_ln + rhs.m_ln;
    return *this;
  }

  operator bool() const;

  string &operator= (string const &);

  friend std::ifstream &operator>> (std::ifstream &in, string &str);
};

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

class Memory {
 protected:
  std::mutex m_mut;
  char      *m_data = nullptr;
  char      *m_rp   = nullptr;
  char      *m_wp   = nullptr;
  size_t     m_size = 0;
  bool       valid  = true;

 public:
  Memory() = default;

  Memory (Memory &&rhs);
  Memory &operator= (Memory &&rhs);

  bool      reserve (size_t n, bool force = false);
  long long write (void const *buf, unsigned n);
};

namespace internal {
class str_iterator {
  char   *m_c;
  string *m_s;

 public:
  str_iterator();
  str_iterator (char &m_c, string &m_s);

  bool          operator== (str_iterator const &rhs) const;
  str_iterator &operator++();

  /* Read */
  operator char const &() const;
  char const &operator*() const;
  char       &operator*();

  /* Write */
  str_iterator &operator= (char c);
};
} // namespace internal

} // namespace scl
#endif