/*  sclreduce.cpp
 */

#include "sclreduce.hpp"
#define LZ4F_STATIC_LINKING_ONLY
#include "lz4/lz4frame.h"

static const LZ4F_preferences_t kPrefs = {
  {LZ4F_max256KB, LZ4F_blockLinked, LZ4F_contentChecksumEnabled, LZ4F_frame,
    0 /* unknown content size */, 0 /* no dictID */, LZ4F_noBlockChecksum},
  2,         /* compression level; 0 == default */
  0,         /* autoflush */
  0,         /* favor decompression speed */
  {0, 0, 0}, /* reserved, must be set to 0 */
};

namespace scl {

reduce_writer::reduce_writer() {
  m_wonly = true;
}

reduce_writer::~reduce_writer() {
}

bool reduce_writer::compress_init() {
  if(!m_inbuf) {
    m_inbuf       = new char[SCL_STREAM_BUF];
    m_outCapacity = LZ4F_compressBound(SCL_STREAM_BUF, &kPrefs);
    m_outbuf      = new char[m_outCapacity];
    const size_t ctxRes =
      LZ4F_createCompressionContext((LZ4F_cctx **)&m_lz4ctx, LZ4F_VERSION);
    if(LZ4F_isError(ctxRes)) {
      if(m_inbuf)
        delete[] m_inbuf;
      if(m_outbuf)
        delete[] m_outbuf;
      m_inbuf  = nullptr;
      m_outbuf = nullptr;
      return false;
    }
  }
  return true;
}

size_t reduce_writer::compress_flush() {
  size_t outSize =
    LZ4F_flush((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity, NULL);
  if(LZ4F_isError(outSize)) {
    return -1;
  }
  if(!write_internal(m_outbuf, outSize, 1))
    return -1;
  return outSize;
}

size_t reduce_writer::compress_chunk(const void *buf, size_t bytes,
  bool flush) {
  size_t outSize;

  if(!m_ready || bytes > SCL_STREAM_BUF)
    return -1;
  if(!bytes)
    return 0;

  outSize = LZ4F_compressUpdate((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity,
    buf, bytes, NULL);
  if(LZ4F_isError(outSize)) {
    return 0;
  }
  if(!write_internal(m_outbuf, outSize, 1))
    return -1;

  if(flush)
    outSize = compress_flush();
  return outSize;
}

long long reduce_writer::read(void *buf, size_t n) {
  // Not needed
  return 0;
}

bool reduce_writer::begin() {
  if(!compress_init())
    return false;
  const size_t headerSize =
    LZ4F_compressBegin((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity, &kPrefs);
  if(LZ4F_isError(headerSize)) {
    return false;
  }
  if(!write_internal(m_outbuf, headerSize, 1)) {
    return false;
  }
  m_ready = true;
  return true;
}

bool reduce_writer::end() {
  if(!m_ready)
    return false;
  size_t outSize =
    LZ4F_compressEnd((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity, NULL);
  if(LZ4F_isError(outSize)) {
    return false;
  }
  write_internal(m_outbuf, outSize, 1);
  m_ready = false;
  return true;
}

bool reduce_writer::write(const void *buf, size_t n, size_t align) {
  for(; n > 0;) {
    size_t inSize = std::min(n, (size_t)SCL_STREAM_BUF);

    if(compress_chunk(buf, inSize, inSize == n) == (size_t)-1)
      return false;

    buf = (char *)buf + inSize;
    n -= inSize;
  }
  return true;
}

bool reduce_writer::write(scl::stream &src) {
  if(!m_ready || !m_inbuf)
    return false;
  for(;;) {
    size_t inSize = src.read(m_inbuf, SCL_STREAM_BUF);
    if(!inSize) {
      return compress_flush() != (size_t)-1;
    }
    if(compress_chunk(m_inbuf, inSize, false) == (size_t)-1)
      return false;
  }
  return true;
}

void reduce_writer::close() {
  if(m_lz4ctx) {
  }
  // Free stream resources.
  stream::close();
}
} // namespace scl
