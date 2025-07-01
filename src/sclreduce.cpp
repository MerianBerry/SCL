/*  sclreduce.cpp
 */

#include "sclreduce.hpp"
#define LZ4F_STATIC_LINKING_ONLY
#include "lz4/lz4frame.h"

static const LZ4F_preferences_t kPrefs = {
  {LZ4F_max64KB, LZ4F_blockLinked, LZ4F_contentChecksumEnabled, LZ4F_frame,
    0 /* unknown content size */, 0 /* no dictID */, LZ4F_noBlockChecksum},
  2,         /* compression level; 0 == default */
  0,         /* autoflush */
  0,         /* favor decompression speed */
  {0, 0, 0}, /* reserved, must be set to 0 */
};

static size_t get_block_size(const LZ4F_frameInfo_t *info) {
  switch(info->blockSizeID) {
  case LZ4F_default:
  case LZ4F_max64KB:
    return 1 << 16;
  case LZ4F_max256KB:
    return 1 << 18;
  case LZ4F_max1MB:
    return 1 << 20;
  case LZ4F_max4MB:
    return 1 << 22;
  default:
    return 0;
  }
}

namespace scl {

bool reduce_stream::compress_init() {
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

size_t reduce_stream::compress_flush() {
  size_t outSize =
    LZ4F_flush((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity, NULL);
  if(LZ4F_isError(outSize)) {
    return -1;
  }
  if(!write_internal(m_outbuf, outSize, 1))
    return -1;
  return outSize;
}

size_t reduce_stream::compress_chunk(const void *buf, size_t bytes,
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

bool reduce_stream::compress_begin() {
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
  return true;
}

bool reduce_stream::compress_end() {
  size_t outSize =
    LZ4F_compressEnd((LZ4F_cctx *)m_lz4ctx, m_outbuf, m_outCapacity, NULL);
  if(LZ4F_isError(outSize)) {
    return false;
  }
  write_internal(m_outbuf, outSize, 1);
  return true;
}

bool reduce_stream::decompress_init() {
  if(!m_inbuf) {
    m_inbuf = new char[SCL_STREAM_BUF];
    const size_t ret =
      LZ4F_createDecompressionContext((LZ4F_dctx **)&m_lz4ctx, LZ4F_VERSION);
    if(LZ4F_isError(ret))
      return false;
  }
  return true;
}

size_t reduce_stream::decompress_chunk(void *buf, size_t bytes) {
  size_t ret     = 1;
  size_t written = 0;
  // Fetch and decompress data into outbuf
  while(ret) {
    // Load more input, if necessary
    if(m_consumed >= m_inSize) {
      m_inSize   = read_internal(m_inbuf, SCL_STREAM_BUF);
      m_consumed = 0;
    }
    size_t      readSize = m_inSize - m_consumed;
    const void *srcptr   = m_inbuf + m_consumed;
    const void *srcend   = (char *)srcptr + readSize;
    if(!readSize)
      break;

    /* Decompress, continueing until srcptr >= srcend, or error */
    while(srcptr < srcend && ret) {
      size_t dstSize = bytes - written;
      size_t srcSize = (char *)srcend - (char *)srcptr;
      ret = LZ4F_decompress((LZ4F_dctx *)m_lz4ctx, (char *)buf + written,
        &dstSize, srcptr, &srcSize, NULL);
      if(LZ4F_isError(ret)) {
        return -1;
      }
      // Update number of written bytes
      written += dstSize;
      // Update input
      srcptr = (char *)srcptr + srcSize;
      m_consumed += srcSize;
      if(written >= bytes)
        break;
    }
    if(written >= bytes || !ret)
      break;
  }
  return written;
}

bool reduce_stream::decompress_begin() {
  if(!decompress_init())
    return false;

  /* Read header */
  const size_t readSize = read_internal(m_inbuf, SCL_STREAM_BUF);
  if(!readSize)
    return false;
  m_inSize   = readSize;
  m_consumed = readSize;

  LZ4F_frameInfo_t info;
  {
    const size_t ret =
      LZ4F_getFrameInfo((LZ4F_dctx *)m_lz4ctx, &info, m_inbuf, &m_consumed);
    if(LZ4F_isError(ret)) {
      return false;
    }
  }

  /* Create m_outbuf with size of frame blocks */
  m_outCapacity = get_block_size(&info);
  if(!m_outCapacity)
    return false;
  m_outbuf = new char[m_outCapacity];
  return true;
}

bool reduce_stream::decompress_end() {
  if(m_outbuf)
    delete[] m_outbuf;
  m_consumed    = 0;
  m_inSize      = 0;
  m_outbuf      = nullptr;
  m_outConsumed = 0;
  m_outSize     = 0;
  m_outCapacity = 0;
  m_ready       = false;
  return true;
}

void reduce_stream::close_internal() {
  end();
  if(m_lz4ctx) {
    if(m_mode == Compress)
      LZ4F_freeCompressionContext((LZ4F_cctx *)m_lz4ctx);
    else
      LZ4F_freeDecompressionContext((LZ4F_dctx *)m_lz4ctx);
  }
  if(m_inbuf)
    delete[] m_inbuf;
  if(m_outbuf)
    delete[] m_outbuf;
  m_lz4ctx      = nullptr;
  m_inbuf       = nullptr;
  m_consumed    = 0;
  m_inSize      = 0;
  m_outbuf      = nullptr;
  m_outConsumed = 0;
  m_outSize     = 0;
  m_outCapacity = 0;
  m_ready       = false;
}

reduce_stream::reduce_stream(reduce_stream &&rhs) {
  stream(std::move(rhs));
  m_inbuf       = rhs.m_inbuf;
  m_outbuf      = rhs.m_outbuf;
  m_lz4ctx      = rhs.m_lz4ctx;
  m_consumed    = rhs.m_consumed;
  m_inSize      = rhs.m_inSize;
  m_outConsumed = rhs.m_outConsumed;
  m_outSize     = rhs.m_outSize;
  m_outCapacity = rhs.m_outCapacity;
  m_ready       = rhs.m_ready;
  m_mode        = rhs.m_mode;
}

reduce_stream::reduce_stream(stream &&rhs) {
  stream(std::move(rhs));
}

reduce_stream &reduce_stream::operator=(reduce_stream &&rhs) {
  stream::operator=(std::move(rhs));
  close_internal();
  m_inbuf       = rhs.m_inbuf;
  m_outbuf      = rhs.m_outbuf;
  m_lz4ctx      = rhs.m_lz4ctx;
  m_consumed    = rhs.m_consumed;
  m_inSize      = rhs.m_inSize;
  m_outConsumed = rhs.m_outConsumed;
  m_outSize     = rhs.m_outSize;
  m_outCapacity = rhs.m_outCapacity;
  m_ready       = rhs.m_ready;
  m_mode        = rhs.m_mode;
  return *this;
}

reduce_stream &reduce_stream::operator=(stream &&rhs) {
  stream::operator=(std::move(rhs));
  close_internal();
  return *this;
}

reduce_stream::~reduce_stream() {
  close_internal();
  stream::~stream();
}

bool reduce_stream::open(const scl::path &path, bool trunc) {
  return stream::open(path, trunc, true);
}

bool reduce_stream::begin(reduce_stream::ReduceMode mode) {
  if(m_ready)
    return mode == m_mode;
  if(m_mode != mode)
    // Reset all reduce resources
    close_internal();
  m_mode = mode;
  if(mode == Compress) {
    if(!compress_init())
      return false;
    m_ready = compress_begin();
  } else {
    if(!decompress_init())
      return false;
    m_ready = decompress_begin();
  }
  if(!m_ready)
    // <mode>_begin() failed, reset resources
    close_internal();
  return m_ready;
}

bool reduce_stream::end() {
  if(!m_ready)
    return false;
  m_ready = false;
  if(m_mode == Compress)
    return compress_end();
  else
    return decompress_end();
}

long long reduce_stream::read(void *buf, size_t n) {
  if(!m_ready || m_mode != Decompress)
    return 0;
  if(m_outSize > m_outCapacity || m_outConsumed > m_outCapacity)
    return 0;
  if(m_outConsumed < m_outSize) {
    size_t avail     = m_outSize - m_outConsumed;
    size_t readBytes = avail > n ? n : avail;
    memcpy(buf, m_outbuf + m_outConsumed, readBytes);
    n -= readBytes;
    m_outConsumed += readBytes;
    buf = (char *)buf + readBytes;
    if(!n || m_outSize < m_outCapacity)
      return readBytes;
  }
  size_t buffered = decompress_chunk(m_outbuf, m_outCapacity);
  if(!buffered)
    return 0;
  m_outSize     = buffered;
  m_outConsumed = 0;
  return read(buf, n);
}

void reduce_stream::flush() {
  if(m_ready && m_mode == Compress)
    compress_flush();
  stream::flush();
}

bool reduce_stream::write(const void *buf, size_t n, size_t align, bool flush) {
  if(!m_ready || m_mode != Compress)
    return false;
  for(; n > 0;) {
    size_t inSize = std::min(n, (size_t)SCL_STREAM_BUF);

    if(compress_chunk(buf, inSize, inSize == n) == (size_t)-1)
      return false;

    if(flush && compress_flush() == (size_t)-1)
      return false;

    buf = (char *)buf + inSize;
    n -= inSize;
  }
  return true;
}

bool reduce_stream::write_uncompressed(const void *buf, size_t n, size_t align,
  bool flush) {
  if(m_ready)
    // Cannot write uncompressed while "active"
    // TODO: allow this behavior during compression (lz4f allows it)
    return false;
  return write_internal(buf, n, align);
}

void reduce_stream::close() {
  // Free stream resources.
  close_internal();
  stream::close();
}
} // namespace scl
