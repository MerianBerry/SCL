/*
  Copyright (c) 2026 Merian

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/* internal.hpp
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
