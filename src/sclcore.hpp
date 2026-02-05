/*  sclcore.hpp
 *  Core scl utilities
 */

#ifndef SCL_CORE_H
#define SCL_CORE_H

#include <fstream>
#include <functional>
#include <algorithm>
#include <stdarg.h>
#include <string.h>

#ifdef max
#  undef max
#endif
#ifdef min
#  undef min
#endif

#define SCL_MAX_REFS 4096
#ifndef SCL_STREAM_BUF
#  define SCL_STREAM_BUF 8192
#endif

/**
 * @brief Main SCL namespace
 *
 */
namespace scl {
class path;
typedef unsigned char uchar;

uchar                 log2i(unsigned x);

/**
 * @brief SCL internal namespace
 *
 */
namespace internal {
class RefObj {
 private:
  int  m_refi = 0;

  bool findslot();
  void incslot() const;
  bool decslot();

 public:
  RefObj();
  RefObj(const RefObj&);
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
  virtual void mutate(bool free) = 0;

  /**
   * @brief Decrements the number of references for this object.
   *
   * @return <b>true</b> if no references remain.
   * @return <b>false</b> if otherwise.
   */
  bool         deref();

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
  bool         make_unique(bool copy = true);

  /**
   * @brief Makes this object reference another. Behaves identically to the
   * RefObj copy constructor.
   * @note This does not handle any managed memory of the derived class, which
   * must be handled manually before using this method.
   */
  void         ref(const RefObj&);

  bool         operator==(const RefObj& rhs) const;

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

  char*    m_buf = nullptr;
  unsigned m_ln  = 0;
  unsigned m_sz  = 0;

  void     mutate(bool free) override;

 public:
  string();
  string(const std::string&);
  string(const char*);
#ifdef _WIN32
  string(const wchar_t*);
#endif
  string(const scl::string&);
  ~string() override;

  /**
   * @brief Clears this strings memory.
   *
   */
  void         clear();

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
   * @param size Size in bytes to set the capacity to.
   */
  scl::string& reserve(unsigned size);

  /**
   * @brief Returns the managed string buffer of this string object. Does not
   * need to be freed.
   * @note If the managed buffer is free'd, the pointer returned by this
   * function will become invalid.
   *
   * @return Pointer to this string's managed buffer. NULL if
   * this string has no contents.
   */
  const char*  cstr() const;
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
  long long        toInt() const;

  /**
   * @brief Returns the length of this string, excluding null terminator.
   *
   * @return Length in bytes.
   */
  unsigned         len() const;

  /**
   * @brief Returns the capacity of this string.
   *
   * @return Capacity in bytes.
   */
  unsigned         size() const;

  /**
   * @brief Finds the first instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the first instance found. -1 if no instance is
   * found.
   */
  long long        ffi(const scl::string& pattern) const;

  /**
   * @brief Finds the last instance of a pattern in this string.
   *
   * @param pattern A string pattern to search for (no wildcard supported).
   * @return Index of the last instance found. -1 if no instance is
   * found.
   */
  long long        fli(const scl::string& pattern) const;

  /**
   * @brief Returns whether or not this string ends with a pattern.
   *
   * @param pattern A string pattern to check with (no wildcard supported).
   * @return true if this string ends with `pattern`, false if otherwise.
   */
  bool             endswith(const scl::string& pattern) const;

  /**
   * @brief Reterns whether or not this string matches a pattern.
   *
   * @param pattern A string pattern to match with (wildcard and ?
   * supported).
   * @return true if this string matches `pattern` at least once, false if
   * otherwise.
   */
  bool             match(const scl::string& pattern) const;

  /**
   * @brief Returns a hash of this string.
   *
   */
  unsigned         hash() const;

  /**
   * @brief Returns a substring of this string.
   *
   * @param i The start index of the substring.
   * @param j The length of the substring.
   */
  scl::string      substr(unsigned i, unsigned j = -1) const;

  /**
   * @brief Returns a unique copy of this string.
   */
  scl::string      copy() const;

  /**
   * @brief Replaces any instance of a pattern in this string with a
   * replacement.
   *
   * @param pattern A string pattern to replace (no wildcard supported).
   * @param with Replacement string.
   */
  scl::string&     replace(const scl::string& pattern, const scl::string& with);

  /**
   * @brief Replaces all lowercase ascii characters with their uppercase
   * counterparts.
   */
  scl::string&     toUpper();

  /**
   * @brief  Finds the first instance of a pattern in the given string.
   *
   * @param  str  String to process.
   * @param  pattern  A string pattern to search for (no wildcard supported).
   * @return  Index of the first instance found. -1 if no instance is
   * found.
   */
  static long long ffi(const char* str, const char* pattern);

  /**
   * @brief  Returns a substring of the given string.
   *
   * @param  str  String to process.
   * @param  i  The start index of the substring.
   * @param  j  The length of the substring.
   */
  static scl::string     substr(const char* str, unsigned i, unsigned j);

  /**
   * @brief Returns a randomized string of a specified length.
   *
   * @param len Length in characters of the randomized string.
   */
  static scl::string     rand(unsigned len);
  static scl::string     vfmt(const char* fmt, va_list args);

  /**
   * @brief Returns a formatted string.
   *
   * @param fmt A C-Style string format.
   * @param ... Formatting arguments.
   */
  static scl::string     fmt(const char* fmt, ...);

  /**
   * @brief  Creates a hash of an scl::string.
   *
   * @param  str  String to hash.
   * @return  Hash.
   */
  static unsigned        hash(const scl::string& str);
  static bool            match(const char* str, const char* pattern);

  /**
   * @return  An iterator to the start of this string.
   */
  internal::str_iterator begin();

  /**
   * @return  An iterator to one past the length of this string.
   */
  internal::str_iterator end();

  /**
   * @brief  Returns an iterator to the given index.
   *
   * @param  long  Index.
   * @return  Iterator to the given index. Returns end() if the index is
   * invalid.
   */
  internal::str_iterator operator[](long long);

  bool                   operator==(const scl::string&) const;
  bool                   operator!=(const scl::string&) const;

  /**
   * @brief  Equivalent to strcmp() < 0
   */
  bool                   operator<(const scl::string&) const;

  scl::string            operator+(const scl::string&) const;

  /**
   * @brief  Concatenates this string and a char.
   *
   * @tparam  step  How to align reserve space. By default 1. If multiple
   * additions occur on this string, increasing this value can dramatically
   * improve performance.
   * @param  rhs  Char to concatenate with.
   */
  template <int step = 1>
  scl::string& operator+=(char rhs) {
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
  template <int step = 1>
  scl::string& operator+=(const scl::string& rhs) {
    if(!rhs)
      return *this;
    make_unique();
    if(!*this || m_ln + rhs.m_ln >= m_sz - 1) {
      const unsigned m   = ((m_ln + rhs.m_ln + (step - 1)) / step);
      unsigned       req = m * step;
      reserve(req);
    }
    memcpy(m_buf + m_ln, rhs.m_buf, (size_t)rhs.m_ln + 1);
    m_ln = m_ln + rhs.m_ln;
    return *this;
  }

                        operator bool() const;

  scl::string&          operator=(const scl::string&);

  friend std::ifstream& operator>>(std::ifstream& in, scl::string& str);
};

std::ostream& operator<<(std::ostream& out, const scl::string& str);

scl::string   operator+(const scl::string& str, const char* str2);

/**
 * @brief Resets the output of scl::clock(), making current time epoch.
 *
 */
void          resetclock();

/**
 * @note You can use scl::resetclock() to control this function's epoch.
 *
 * @return   Seconds since epoch.
 */
double        clock();

/**
 * @brief Makes this thread sleep for a given amount of milliseconds.
 *
 * @param sleemms  Number of milliseconds to sleep for.
 */
void          waitms(double ms);

/**
 * @brief  Waits until the given lamda function returns true.
 *
 * @param  cond  Lambda function to be called.
 * @param  timeout  Max number of seconds to wait. By default -1 (infinite).
 * @param  sleepms  Number of milliseconds to sleep for inbetween condition
 * checks. By default 0.001ms.
 * @return  true: Wait did not time out, false: Wait did time out.
 */
bool          waitUntil(std::function<bool()> cond, double timeout = -1,
           double sleepms = 0.001);

enum class StreamPos {
  start   = SEEK_SET,
  end     = SEEK_END,
  current = SEEK_CUR,
};

enum class OpenMode {
  // Read only. Fails if file isnt present.
  READ = 0,
  // Write only. Truncates file if it exists, or creates it if it doesnt.
  WRITE = 1,
  // Read/Write. Fails if file isnt present.
  RW = 2,
  // Read/Write. Truncates file if it exists, or creates it if it doesnt.
  RWTRUNC = 3,
  // Append only. Can only append content, and creates the file it it doesnt
  // exist.
  APPEND = 4,
  // Read/Append. Can only append content, and creates the file it it doesnt
  // exist.
  RAPPEND = 5,
};

class stream {
 protected:
  FILE*     m_stream = nullptr;
  char*     m_data   = nullptr;
  char*     m_fp     = nullptr;
  size_t    m_size   = 0;
  bool      m_ronly = false, m_wonly = false;
  bool      m_modified = false;

  long long bounds(const char* p, size_t n) const;

  long long read_internal(void* buf, size_t n);
  bool      write_internal(const void* buf, size_t n, size_t align);
  void      close_internal();

 public:
  stream()                  = default;
  stream(const stream& rhs) = delete;
  stream(stream&& rhs);
  stream& operator=(const stream& rhs) = delete;
  stream& operator=(stream&& rhs);

  virtual ~stream();

  /**
   * @return  true if this stream is in file mode, and target file was opened
   * successfully.
   */
  bool         is_open() const;

  /**
   * @return  true if this stream has been written to.
   */
  bool         is_modified() const;

  /**
   * @return  Offset in bytes of the rw pointer.
   */
  long long    tell() const;

  /**
   * @brief  Resets the modified status of this stream to false.
   *
   */
  void         reset_modified();

  /**
   * @brief  Opens this stream to a path, with a specific mode.
   *
   * @param  path  Path to open.
   * @param  mode  Open mode. See C fopen modes.
   * @return  true if the operation was successful.
   */
  bool         openMode(const scl::path& path, const scl::string& mode);

  /**
   * @brief  Opens this stream to a path.
   *
   * @param  path  Path to open.
   * @param  mode  The mode to use when opening the file.
   * @param  binary  Whether or not to open in binary mode (/r/n -> /n while
   * reading in non-binary mode).
   * @return  true if the operation was successful.
   */
  bool         open(const scl::path& path, OpenMode mode, bool binary = false);

  /**
   * @brief  Used to flush internal buffers. Does nothing in memory mode.
   *
   */
  virtual void flush();

  /**
   * @brief  Moves the rw pointer to the given position.
   *
   * @param  pos  Seek position.
   * @param  off  Offset from the given seek position.
   * @return  New offset of the rw pointer. Equivalent to calling tell() right
   * after this method call.
   */
  long long    seek(StreamPos pos, long long off);

  /**
   * @brief  Reads `n` bytes from this stream into `buf`. If not enough bytes
   * are able to be read, returns the number of read bytes. If the operation
   * errored, or there is nothing left to read, returns 0.
   *
   * @param  buf  Buffer to store read data.
   * @param  n  Number of bytes to read.
   * @return  Number of bytes read, 0 if nothing was read, or if an error
   * occured.
   */
  virtual long long read(void* buf, size_t n);

  /**
   * @brief  Reserves space while in memory mode. Reserves space starting at the
   * rw pointer, not buffer start. By default does nothing if there is enough
   * space remaining.
   *
   * @param  n  Number of bytes to reserve.
   * @param  force  Reserve space, even if there is enough space.
   * @return  true if the operation was successful.
   */
  bool              reserve(size_t n, bool force = false);

  /**
   * @brief  Writes a memory buffer to this stream.
   *
   * @param  buf  Buffer to write.
   * @param  n  Number of bytes to write.
   * @param  align  How to align reserve space. By default 1. If a multitude of
   * writes occur to this stream, increasing this value can dramatically
   * increase performance.
   * @param  flush  true: Automatically calls flush().
   * @return  true if the operation was successful, and the requested number of
   * bytes were written.
   */
  virtual bool      write(const void* buf, size_t n, size_t align = 1,
         bool flush = false);

  /**
   * @brief  Writes an scl::string's length to this stream.
   *
   * @param  str  String to write.
   * @param  align  How to align reserve space. By default 1. If a multitude of
   * writes occur to this stream, increasing this value can dramatically
   * increase performance.
   * @param  flush  true: Automatically calls flush().
   * @return  true if the operation was successful, and the requested number of
   * bytes were written.
   */
  bool write(const scl::string& str, size_t align = 1, bool flush = false);

  /**
   * @brief  Writes another scl::string into this stream.
   *
   * @param  src  Stream to read from.
   * @param  max  Max number of bytes to write. By default -1 (infinite).
   * @return  true if the operation was successful.
   */
  bool write(scl::stream& src, size_t max = -1);

  /**
   * @brief  Closes this stream. Closes the file in file mode, and releases
   * buffers in memory mode.
   *
   */
  virtual void close();

  /**
   * @return   Returns a pointer to the internal data buffer, if in memory mode.
   * Returns nullptr if operating in file mode.
   * @warning Do not free the pointer that is returned by this method. And it is
   * possible for this pointer to be invalidated, if the stream owning it
   * releases it.
   */
  const void*  data();

  /**
   * @brief  Releases the internal data buffer from this streams control, if in
   * memory mode. This stream will be reset after this call.
   *
   * @return  Pointer to this streams internal data buffer. If valid, you must
   * free it.
   */
  void*        release();

  stream&      operator<<(const scl::string& str);
  stream&      operator>>(scl::string& str);
};

namespace internal {
class str_iterator {
  string*  m_s = nullptr;
  unsigned m_i = -1;

 public:
  str_iterator() = default;
  str_iterator(scl::string& m_s, unsigned i);

  bool          operator==(const str_iterator& rhs) const;
  str_iterator& operator++();

  /* Read */
                operator const char&() const;
  const char&   operator*() const;

  /* Write */
                operator char&();
  char&         operator*();
  str_iterator& operator=(char c);
};
} // namespace internal

/**
 * @brief Initializes SCL global resources. Libraries currently requiring this
 * is scl::pack.
 *
 * @return  Returns true on success, false on otherwise.
 */
bool init();

/**
 * @brief Releases SCL global resources. Libraries currently requiring this
 * is scl::pack.
 *
 */
void terminate();
} // namespace scl
#endif
