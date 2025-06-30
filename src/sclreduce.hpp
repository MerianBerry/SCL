/*  sclreduce.hpp
 *  Compression stream class.
 */

#include "sclcore.hpp"

namespace scl {
class reduce_writer : public stream {
 private:
  char  *m_inbuf = nullptr, *m_outbuf = nullptr;
  void  *m_lz4ctx      = nullptr;
  size_t m_outCapacity = 0;
  bool   m_ready       = false;

  bool   compress_init();
  size_t compress_flush();
  size_t compress_chunk(const void *buf, size_t bytes, bool flush);

 public:
  reduce_writer();
  ~reduce_writer() override;

  // long long seek (StreamPos pos, long long off) override;

  long long read(void *buf, size_t n) override;

  bool      begin();
  bool      end();

  using stream::write;
  bool write(const void *buf, size_t n, size_t align) override;
  bool write(scl::stream &src) override;

  void close() override;
};
} // namespace scl
