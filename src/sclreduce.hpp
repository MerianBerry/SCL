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

  bool      open(const scl::path &path, bool trunc = false);

  bool      begin(ReduceMode mode = Decompress);
  bool      end();

  long long read(void *buf, size_t n) override;

  void      flush() override;
  using stream::write;
  bool write(const void *buf, size_t n, size_t align,
    bool flush = true) override;
  bool write_uncompressed(const void *buf, size_t n, size_t align = 1,
    bool flush = true);

  void close() override;
};
} // namespace scl

#endif
