/* stream.cpp
 * scl::stream class
 * IO and memory buffer abstraction container
 */

#include <scl_stream.hpp>
#include "internal.hpp"

namespace scl {
stream::stream(stream&& rhs) {
  m_stream = rhs.m_stream;
  m_data = rhs.m_data;
  m_fp = rhs.m_fp;
  m_size = rhs.m_size;
  m_ronly = rhs.m_ronly;
  m_modified = rhs.m_modified;

  rhs.m_stream = 0;
  rhs.m_data = 0;
  rhs.m_fp = 0;
  rhs.m_size = 0;
  rhs.m_ronly = 0;
  rhs.m_modified = 0;
}

stream& stream::operator=(stream&& rhs) {
  m_stream = rhs.m_stream;
  m_data = rhs.m_data;
  m_fp = rhs.m_fp;
  m_size = rhs.m_size;
  m_ronly = rhs.m_ronly;
  m_modified = rhs.m_modified;

  rhs.m_stream = 0;
  rhs.m_data = 0;
  rhs.m_fp = 0;
  rhs.m_size = 0;
  rhs.m_ronly = 0;
  rhs.m_modified = 0;
  return *this;
}

stream::stream(const scl::path& path, OpenMode mode, bool binary) : stream() {
  this->open(path, mode, binary);
}

void stream::close_internal() {
  flush();
  if(m_stream)
    fclose(m_stream);
  if(m_data)
    delete[] m_data;
  m_stream = 0;
  m_data = 0;
  m_fp = 0;
  m_size = 0;
  m_ronly = 0;
  m_modified = 0;
}

stream::~stream() {
  close_internal();
}

size_t stream::bounds(const char* p, size_t n) const {
  if(p >= m_data + m_size)
    return 0;
  return std::min(n, m_size - (p - m_data));
}

size_t stream::read_internal(void* buf, size_t n) {
  if(m_stream) {
    if(n == (size_t)-1) {
      ptrdiff_t o = (ptrdiff_t)tell();
      seek(StreamPos::end, 0);
      size_t l = tell() - o;
      seek(StreamPos::start, o);
      n = std::min(n, (size_t)l);
    }
    if(feof(m_stream))
      return 0;
    auto r = fread(buf, 1, n, m_stream);
    // Suggested by gnu.org, cause r+/w+ modes are weird
    fflush(m_stream);
    return r;
  }
  size_t r = bounds(m_fp, n);
  if(!r)
    return 0;
  memcpy(buf, m_fp, r);
  m_fp += r;
  return r;
}

#define alignup(x, align) ((((x) + ((align) - 1)) / (align)) * align)

bool stream::write_internal(const void* buf, size_t n, size_t align) {
  if(!buf)
    return false;
  if(m_stream) {
    size_t wr = fwrite(buf, 1, n, m_stream);
    bool r = !n || wr;
    // Suggested by gnu.org, cause r+/w+ modes are weird
    fflush(m_stream);
    return r;
  }
  if(n > (m_size - (m_fp - m_data))) {
    size_t res = alignup(m_size + n, align) - m_size;
    if(!reserve(res))
      return false;
  }
  memcpy(m_fp, buf, n);
  m_fp = std::min(m_fp + n, m_data + m_size);
  m_modified = true;
  return true;
}

bool stream::is_open() const {
  return !!m_stream;
}

bool stream::is_modified() const {
  return m_modified;
}

size_t stream::tell() const {
  ptrdiff_t r;
  if(m_stream)
    r = ftell(m_stream);
  else
    r = m_fp - m_data;
  if(r < 0)
    throw std::runtime_error("scl::stream::tell error");
  return (size_t)r;
}

size_t stream::size() const {
  if(m_stream)
    return 0;
  return m_size;
}

void stream::reset_modified() {
  m_modified = false;
}

bool stream::openMode(const scl::path& path, const scl::string& mode) {
  if(m_stream)
    return false;
  m_ronly = mode == "r" || mode == "rb" || m_ronly;
  m_wonly = mode == "w" || mode == "wb" || mode == "a" || m_wonly;
#ifdef _MSC_VER
#  pragma warning(disable : 4996)
#endif
  m_stream = fopen(path.cstr(), mode.cstr());
  if(m_data && m_stream)
    flush();
  return !!m_stream;
}

bool stream::open(const scl::path& path, OpenMode mode, bool binary) {
  // w+ creates the file, and truncates, and allows fseek to read and write.
  // r+ doesnt truncate the file, and allows fseek to read and write.
  scl::string smode;
  switch(mode) {
  case OpenMode::READ:
    smode = "r";
    break;
  case OpenMode::WRITE:
    smode = "w";
    break;
  case OpenMode::RW:
    smode = "r+";
    break;
  case OpenMode::RWTRUNC:
    smode = "w+";
    break;
  case OpenMode::APPEND:
    smode = "a";
    break;
  case OpenMode::RAPPEND:
    smode = "a+";
    break;
  }
  if(binary)
    smode = smode.substr(0, 1) + "b" + smode.substr(1);
  return openMode(path, smode);
}

void stream::flush() {
  if(m_data && m_stream) {
    write_internal(m_data, m_size, 1);
    delete[] m_data;
    m_data = nullptr;
    m_fp = nullptr;
    m_size = 0;
  }
  if(m_stream)
    fflush(m_stream);
}

size_t stream::seek(StreamPos pos, ptrdiff_t off) {
  if(m_stream) {
    fseek(m_stream, (long)off, (int)pos);
    return tell();
  }
  if(pos == StreamPos::start)
    m_fp = m_data;
  else if(pos == StreamPos::end)
    m_fp = m_data + m_size;
  m_fp += off;
  m_fp = std::max(m_fp, m_data);
  return tell();
}

size_t stream::read(void* buf, size_t n) {
  if(m_wonly)
    return 0;
  return read_internal(buf, n);
}

bool stream::reserve(size_t n, bool force) {
  // Ignore file mode
  if(m_stream)
    return true;
  // Remaining length
  ptrdiff_t rl = (m_data + m_size) - m_fp;
  if(rl < (ptrdiff_t)n || force) {
    size_t nsz = m_size + n;
    ptrdiff_t foff = m_fp - m_data;

    char* buf = new char[nsz];
    if(!buf)
      return false;
    if(m_data) {
      memcpy(buf, m_data, m_size);
      delete[] m_data;
    }
    memset(buf + m_size, 0, nsz - m_size);
    m_fp = buf + foff;
    m_data = buf;
    m_size = nsz;
  }
  return true;
}

bool stream::write(const void* buf, size_t n, size_t align, bool flush) {
  if(m_ronly)
    return false;
  // Just ignore flush?
  return write_internal(buf, n, align);
}

bool stream::write(const scl::string& str, size_t align, bool flush) {
  return write(str.cstr(), (size_t)str.len(), align, flush);
}

bool stream::write(stream& src, size_t max) {
  char buf[SCL_STREAM_BUF];
  size_t total = 0;
  bool r = true;
  do {
    if(total >= max)
      break;
    size_t read = src.read(buf, SCL_STREAM_BUF);
    size_t avail = max - total;
    size_t readBytes = avail < read ? avail : read;
    total += read;
    if(readBytes)
      // Write. Flush if the streaming buffer isnt full (usually end of
      // streaming).
      r = write(buf, readBytes, 1, readBytes < SCL_STREAM_BUF);
    if(!readBytes || !r)
      break;
  } while(1);
  return r;
}

void stream::close() {
  close_internal();
}

const void* stream::data() {
  return m_data;
}

void* stream::release() {
  void* ptr = nullptr;
  if(m_data) {
    ptr = m_data;
    // Make sure the data ptr isnt freed
    m_data = nullptr;
    // Reset members
    close_internal();
  }
  return ptr;
}

stream& stream::operator<<(const scl::string& str) {
  write(str);
  return *this;
}

stream& stream::operator>>(scl::string& str) {
  ptrdiff_t off = (ptrdiff_t)tell();
  size_t end = seek(StreamPos::end, 0);
  seek(StreamPos::start, off);
  if(end >= UINT_MAX)
    return *this;
  char* buf = new char[SCL_STREAM_BUF];
  str.reserve((int32_t)end);
  for(;;) {
    auto readBytes = read(buf, SCL_STREAM_BUF - 1);
    if(!readBytes)
      break;
    buf[readBytes] = 0;
    str.operator+= <256>(buf);
  }
  delete[] buf;
  return *this;
}
} // namespace scl
