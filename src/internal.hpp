/* defs.hpp
 * Core scl definitions
 */

#ifndef scl_defs_hpp
#define scl_defs_hpp

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifndef SCL_STREAM_BUF
#  define SCL_STREAM_BUF 0x8000
#endif

#ifndef SPK_MAX_PACK_SIZE
#  define SPK_MAX_PACK_SIZE 0xffffffff
#endif

// PLATFORM DEFINITIONS

#ifndef NOMINMAX
#  define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif

namespace scl {
namespace internal {

uint64_t fasthash64(const void* m_buf, size_t len, uint64_t seed);

}
} // namespace scl

#endif
