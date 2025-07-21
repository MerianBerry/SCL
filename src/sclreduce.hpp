/*  sclreduce.hpp
 *  Compression stream class.
 */

#ifndef SCL_REDUCE_H
#define SCL_REDUCE_H
#include "sclcore.hpp"

namespace scl {
class reduce_stream : public stream {
 public:
  enum ReduceMode : bool {
    Decompress = false,
    Compress   = true,
  };

 private:
  char  *m_inbuf = nullptr, *m_outbuf = nullptr;
  void  *m_lz4ctx   = nullptr;
  size_t m_consumed = 0, m_inSize = 0, m_outConsumed = 0, m_outSize = 0,
         m_outCapacity = 0;
  bool       m_ready   = false;
  ReduceMode m_mode    = Decompress;

  bool       compress_init();
  size_t     compress_flush();
  size_t     compress_chunk(const void *buf, size_t bytes, bool flush);
  bool       compress_begin();
  bool       compress_end();

  bool       decompress_init();
  size_t     decompress_chunk(void *buf, size_t bytes);
  bool       decompress_begin();
  bool       decompress_end();

  void       close_internal();


 public:
  reduce_stream() = default;

  reduce_stream(reduce_stream &&rhs);
  reduce_stream(stream &&rhs);
  reduce_stream &operator=(reduce_stream &&rhs);
  reduce_stream &operator=(stream &&rhs);

  ~reduce_stream() override;

  /**
   * @brief  Opens a file.
   *
   * @param  path  Path to open.
   * @param  trunc  Whether or not to truncate (erase) existing file contents.
   * @return  true if the operation was successful.
   */
  bool      open(const scl::path &path, bool trunc = false);

  /**
   * @brief  Begins either a decompression or compression state.
   * @warning Will fail if a
   * state is already active (see end()).
   * @note  If reading, this only accepts compressed data written by this class,
   * aka LZ4's frame format. If the data present in this stream is not in LZ4's
   * frame format, this method will fail.
   *
   * @param  mode  Mode to begin with. Defaults to decompression.
   * @return  true if the operation was successful.
   */
  bool      begin(ReduceMode mode = Decompress);

  /**
   * @brief  Ends the previously started decompression/compression state, and
   * flushes internal buffers.
   *
   * @return  true if the operation was successful.
   */
  bool      end();

  /**
   * @brief  Reads and decompresses data from this stream.
   * @warning  A decompression state must be started before this call (see
   * begin()), or else it will fail. This method does not end the decompression
   * state, and can be used as many times as you want as long as the
   * decompression state is active.
   *
   * @param  buf  Buffer to write decompressed data into.
   * @param  n  Number of decompressed bytes to write into `buf`.
   * @return  Number of decompressed bytes read. 0 if the operation errored, or
   * there was nothing to read.
   */
  long long read(void *buf, size_t n) override;

  /**
   * @brief  Flushes internal buffers.
   *
   */
  void      flush() override;
  using stream::write;
  /**
   * @brief  Compresses and write data to this stream.
   * @warning  A compression state must be started before this call (see
   * begin()), or else it will fail. This method does not end the compression
   * state, and can be used as many times as you want as long as the
   * compression state is active.
   *
   * @param  buf  Buffer to compress and write.
   * @param  n  Number of uncompressed bytes to write.
   * @param  align  How to align reserve space. By default 1. If multiple
   * writes occur to this stream, increasing this value can dramatically
   * increase performance.
   * @param  flush  true: Automatically calls flush(). By default true.
   * @return  true if the operation was successful.
   */
  bool write(const void *buf, size_t n, size_t align = 1,
    bool flush = true) override;

  /**
   * @brief  Writes uncompressed data to this stream. Equivalent to calling
   * scl::stream::write() instead.
   * @warning This method can only be used while a decompression/compression
   * state is not active, or else it will fail.
   *
   * @param  buf  Buffer to write.
   * @param  n  Number of bytes to write.
   * @param  align  How to align reserve space. By default 1. If multiple
   * writes occur to this stream, increasing this value can dramatically
   * increase performance.
   * @param  flush  true: Automatically calls flush(). By default true.
   * @return  true if the operation was successful.
   */
  bool write_uncompressed(const void *buf, size_t n, size_t align = 1,
    bool flush = true);

  /**
   * @brief  Closes this stream. Also ends decompression/compression states if
   * they are active.
   *
   */
  void close() override;
};
} // namespace scl

#endif
