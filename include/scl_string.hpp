/* scl_string.hpp
 * scl::string class
 * utf-8 string container
 */

#ifndef scl_string_hpp
#define scl_string_hpp

#include <fstream>
#include <stdarg.h>
#include <string.h>

namespace scl {
class string;

class _str_iterator {
  string* m_s = nullptr;
  int32_t m_i = -1;

public:
  _str_iterator() = default;
  _str_iterator(scl::string& m_s, int32_t i);

  bool operator==(const _str_iterator& rhs) const;
  _str_iterator& operator++();

  /* Read */
  operator const char&() const;
  const char& operator*() const;

  /* Write */
  operator char&();
  char& operator*();
  _str_iterator& operator=(char c);
};

class _str_const_iterator {
  const string* m_s = nullptr;
  int32_t m_i = -1;

public:
  _str_const_iterator() = default;
  _str_const_iterator(const scl::string& m_s, int32_t i);

  bool operator==(const _str_const_iterator& rhs) const;
  _str_const_iterator& operator++();

  /* Read */
  operator const char&() const;
  const char& operator*() const;
};

class string {
private:
  friend class _str_iterator;
  friend class _str_const_iterator;

  // If m_buf is a view, m_sz will be 0, while m_buf will be non-zero.
  // In this case, m_ln will also represent m_sz.
  char* m_buf = nullptr;
  int32_t m_ln = 0;
  int32_t m_sz = 0;

  bool isview() const;
  void make_unique();

public:
  string();
  string(const std::string&);
  string(const char*);
#ifdef _WIN32
  string(const wchar_t*);
#endif
  string(const scl::string&);
  ~string();

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
  scl::string& claim(const char* ptr);

  /**
   * @brief Turns this string into a readonly layer above a given string buffer.
   *
   * @param ptr Pointer to string buffer to view.
   */
  scl::string& view(const char* ptr);

  /**
   * @brief Increases the capacity of this string object, copying the original
   * contents. Can be used to shrink its capacity.
   *
   * @param newSize Size in bytes to set the capacity to.
   */
  scl::string& reserve(int32_t newSize);

  /**
   * @brief Returns the managed string buffer of this string object. Does not
   * need to be freed.
   * @note If the string object managing this pointer is destroyed, the pointer
   * returned by this function will become invalid.
   *
   * @return Pointer to this string's managed buffer. NULL if
   * this string has no contents.
   */
  const char* cstr() const;
#ifdef _WIN32
  /**
   * @brief Returns a wchar_t version of this string.
   * @note The buffer returned by this function is NOT managed, and must be
   * freed manually.
   *
   * @return Pointer to a wchar_t string buffer. NULL if
   * the function failed.
   */
  const wchar_t* wstr() const;
#endif

  /**
   * @brief Attempts to convert as much of this string into an integer as
   * possible. Automatically detects hexedecimal literals.
   *
   * @return  An integer representation of this string.
   */
  int64_t toInt() const;

  /**
   * @brief Returns the byte length of this string, excluding null terminator.
   *
   * @return Length in bytes.

   * @see charlen() for character length instead.
   */
  int32_t len() const;


  /**
   * @brief Returns the utf-8 length of this string, excluding null terminator.
   *
   * @return Length in characters.
   */
  int32_t charlen() const;

  /**
   * @brief Returns the capacity of this string.
   *
   * @return Capacity in bytes.
   */
  int32_t size() const;

  /**
   * @brief Finds the first instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the first instance found. -1 if no instance is
   * found.
   */
  int32_t ffi(const scl::string& pattern) const;

  /**
   * @brief Finds the last instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the last instance found. -1 if no instance is
   * found.
   */
  int32_t fli(const scl::string& pattern) const;

  /**
   * @brief Returns whether or not this string ends with a pattern.
   *
   * @param pattern A string pattern to check with (no wildcard supported).
   * @return true if this string ends with `pattern`, false if otherwise.
   */
  bool endswith(const scl::string& pattern) const;

  /**
   * @brief Reterns whether or not this string matches a pattern.
   *
   * @param pattern A string pattern to match with (wildcard '*'
   * supported).
   * @return true if this string matches `pattern` at least once, false if
   * otherwise.
   */
  bool match(const scl::string& pattern) const;

  /**
   * @brief Returns a hash of this string.
   *
   */
  uint64_t hash() const;

  /**
   * @brief Returns a substring of this string.
   *
   * @param i The start index in characters of the substring.
   * @param j The length in characters of the substring. Default INT_MAX (until
   * end of string).
   */
  scl::string substr(int32_t i, int32_t j = INT_MAX) const;

  /**
   * @brief Returns a unique copy of this string.
   */
  scl::string copy() const;

  /**
   * @brief Replaces any instance of a pattern in this string with a
   * replacement.
   *
   * @param pattern A string pattern to replace (wildcard not supported).
   * @param with Replacement string.
   */
  scl::string& replace(const scl::string& pattern, const scl::string& with);

  /**
   * @brief Replace the substring starting at i, and j bytes long with the
   * string paramater `with`.
   *
   * @param  with  string to place.
   * @param  i  start index in characters.
   * @param  j  length in characters. Default INT_MAX (until end of string).
   * @return Reference to this object.
   */
  scl::string& replace(const scl::string& with, int32_t i, int32_t j = INT_MAX);

  /**
   * @brief Replaces all lowercase ascii characters with their uppercase
   * counterparts.
   */
  scl::string& toUpper();

  /**
   * @brief Replaces all uppercase ascii characters with their lowercase
   * counterparts.
   */
  scl::string& toLower();

  /**
   * @brief  Finds the first instance of a pattern in the given string.
   *
   * @param  str  String to process.
   * @param  pattern  A string pattern to search for (no wildcard supported).
   * @return  Index of the first instance found. -1 if no instance is
   * found.
   */
  static int32_t ffi(const char* str, const char* pattern);

  /**
   * @brief  Finds the last instance of a pattern in the given string.
   *
   * @param  str  String to process.
   * @param  pattern  A string pattern to search for (no wildcard supported).
   * @return  Index of the last instance found. -1 if no instance is
   * found.
   */
  static int32_t fli(const char* str, const char* pattern);

  /**
   * @brief  Returns a substring of the given string.
   *
   * @param  str  String to process.
   * @param  i  The start index in characters of the substring.
   * @param  j  The length in characters of the substring.
   */
  static scl::string substr(const char* str, int32_t i, int32_t j);

  /**
   * @brief Returns a randomized string of a specified length.
   *
   * @param len Length in characters of the randomized string.
   */
  static scl::string rand(int32_t len);
  static scl::string vfmt(const char* fmt, va_list args);

  /**
   * @brief Returns a formatted string.
   *
   * @param fmt A C-Style string format.
   * @param ... Formatting arguments.
   */
  static scl::string fmt(const char* fmt, ...);

  /**
   * @brief  Creates a hash of an scl::string.
   *
   * @param  str  String to hash.
   * @return  Hash.
   */
  static uint64_t hash(const scl::string& str);
  static bool match(const char* str, const char* pattern);

  /**
   * @return  An iterator to the start of this string.
   */
  _str_iterator begin();

  /**
   * @return  An iterator to the start of this string.
   */
  _str_const_iterator cbegin() const;

  /**
   * @return  An iterator to one past the length of this string.
   */
  _str_iterator end();

  /**
   * @return  An iterator to one past the length of this string.
   */
  _str_const_iterator cend() const;

  /**
   * @brief  Returns an iterator to the given index.
   *
   * @param  long  Index in characters.
   * @return  Iterator to the given index. Returns end() if the index is
   * invalid.
   */
  _str_iterator operator[](int32_t);

  /**
   * @brief  Returns an iterator to the given index.
   *
   * @param  long  Index in characters.
   * @return  Iterator to the given index. Returns end() if the index is
   * invalid.
   */
  _str_const_iterator operator[](int32_t) const;

  bool operator==(const scl::string&) const;
  bool operator!=(const scl::string&) const;

  /**
   * @brief  Equivalent to strcmp() < 0
   */
  bool operator<(const scl::string&) const;

  scl::string operator+(const scl::string&) const;

  /**
   * @brief  Concatenates this string and a char.
   *
   * @tparam  step  How to align reserve space. By default 1. If multiple
   * additions occur on this string, increasing this value can dramatically
   * improve performance.
   * @param  rhs  Char to concatenate with.
   */
  template <int step = 1> scl::string& operator+=(char rhs) {
    char s[2] = {rhs, '\0'};
    return (*this).operator+= <step>(scl::string().view(s));
  }

  /**
   * @brief  Concatenates this string and another string.
   *
   * @tparam  step  How to align reserve space. By default 1. If multiple
   * additions occur on this string, increasing this value can dramatically
   * improve performance.
   * @param  rhs  String to concatenate with.
   */
  template <int step = 1> scl::string& operator+=(const scl::string& rhs) {
    if(!rhs)
      return *this;
    make_unique();
    if(!*this || m_ln + rhs.m_ln >= m_sz - 1) {
      const int32_t m = ((m_ln + rhs.m_ln + (step - 1)) / step);
      int32_t req = m * step;
      reserve(req);
    }
    memcpy(m_buf + m_ln, rhs.m_buf, (size_t)rhs.m_ln + 1);
    m_ln = m_ln + rhs.m_ln;
    return *this;
  }

  operator bool() const;

  scl::string& operator=(const scl::string&);

  friend std::ifstream& operator>>(std::ifstream& in, scl::string& str);
};

std::ostream& operator<<(std::ostream& out, const scl::string& str);

scl::string operator+(const scl::string& str, const char* str2);


} // namespace scl

// std::hash override for scl::string
namespace std {
template <> struct hash<scl::string> {
  size_t operator()(const scl::string& str) const noexcept {
    return str.hash();
  }
};
} // namespace std
#endif
