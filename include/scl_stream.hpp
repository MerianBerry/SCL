/* scl_stream.hpp
 * scl::stream class
 * IO and memory buffer abstraction container
 */

#ifndef scl_stream_hpp
#define scl_stream_hpp

#include "scl_string.hpp"
#include "scl_path.hpp"

namespace scl {
enum class StreamPos {
  start = SEEK_SET,
  end = SEEK_END,
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
  FILE* m_stream = nullptr;
  char* m_data = nullptr;
  char* m_fp = nullptr;
  size_t m_size = 0;
  bool m_ronly = false, m_wonly = false;
  bool m_modified = false;

  size_t bounds(const char* p, size_t n) const;

  size_t read_internal(void* buf, size_t n);
  bool write_internal(const void* buf, size_t n, size_t align);
  void close_internal();

public:
  stream() = default;
  stream(const stream& rhs) = delete;
  stream(stream&& rhs);
  stream& operator=(const stream& rhs) = delete;
  stream& operator=(stream&& rhs);
  stream(const scl::path& path, OpenMode mode, bool binary = false);

  virtual ~stream();

  /**
   * @return  true if this stream is in file mode, and target file was opened
   * successfully.
   */
  bool is_open() const;

  /**
   * @return  true if this stream has been written to.
   */
  bool is_modified() const;

  /**
   * @return  Offset in bytes of the rw pointer.
   */
  size_t tell() const;

  /**
   * @brief Returns the current capacity of the stream.
   * Only works in data mode, returns 0 in file mode.
   */
  size_t size() const;

  /**
   * @brief  Resets the modified status of this stream to false.
   *
   */
  void reset_modified();

  /**
   * @brief  Opens this stream to a path, with a specific mode.
   *
   * @param  path  Path to open.
   * @param  mode  Open mode. See C fopen modes.
   * @return  true if the operation was successful.
   */
  bool openMode(const scl::path& path, const scl::string& mode);

  /**
   * @brief  Opens this stream to a path.
   *
   * @param  path  Path to open.
   * @param  mode  The mode to use when opening the file.
   * @param  binary  Whether or not to open in binary mode (/r/n -> /n while
   * reading in non-binary mode).
   * @return  true if the operation was successful.
   */
  bool open(const scl::path& path, OpenMode mode, bool binary = false);

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
  size_t seek(StreamPos pos, ptrdiff_t off);

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
  virtual size_t read(void* buf, size_t n);

  /**
   * @brief  Reserves space while in memory mode. Reserves space starting at the
   * rw pointer, not buffer start. By default does nothing if there is enough
   * space remaining.
   *
   * @param  n  Number of bytes to reserve.
   * @param  force  Reserve space, even if there is enough space.
   * @return  true if the operation was successful.
   */
  bool reserve(size_t n, bool force = false);

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
  virtual bool write(
    const void* buf, size_t n, size_t align = 1, bool flush = false);

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
   * @brief  Writes another scl::stream into this stream.
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
  const void* data();

  /**
   * @brief  Releases the internal data buffer from this streams control, if in
   * memory mode. This stream will be reset after this call.
   *
   * @return  Pointer to this streams internal data buffer. If valid, you must
   * free it.
   */
  void* release();

  stream& operator<<(const scl::string& str);
  stream& operator>>(scl::string& str);
};
} // namespace scl
#endif
