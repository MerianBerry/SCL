/* scl_misc.hpp
 * Misc utilities
 */

#ifndef scl_misc_hpp
#define scl_misc_hpp

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#if defined(_WIN32)
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

namespace scl {

#define scl_vstkfmt(buf, mx, fmt, args)                             \
  {                                                                 \
    va_list copy;                                                   \
    va_copy(copy, args);                                            \
    int __l = std::min(vsnprintf(NULL, 0, fmt, copy), (int)mx - 1); \
    va_end(copy);                                                   \
    if(!(buf))                                                      \
      (buf) = alloca(__l + 1);                                      \
    vsnprintf((char*)(buf), __l + 1, fmt, args);                    \
    (buf)[__l] = 0;                                                 \
  }
#define scl_stkfmt(buf, mx, fmt, ...)                                     \
  {                                                                       \
    int __l = std::min(snprintf(NULL, 0, fmt, __VA_ARGS__), (int)mx - 1); \
    if(!(buf))                                                            \
      (buf) = alloca(__l + 1);                                            \
    snprintf((char*)(buf), __l + 1, fmt, __VA_ARGS__);                    \
    ((char*)(buf))[__l] = 0;                                              \
  }

#define scl_vstkfmt2(buf, mx, fmt, args)                         \
  {                                                              \
    int __l = vsnprintf((char*)(buf), (int)(mx) - 1, fmt, args); \
    ((char*)(buf))[std::min(__l, (int)(mx) - 1)] = 0;            \
  }
#define scl_stkfmt2(buf, mx, fmt, ...)                                 \
  {                                                                    \
    int __l = snprintf((char*)(buf), (int)(mx) - 1, fmt, __VA_ARGS__); \
    ((char*)(buf))[std::min(__l, (int)(mx) - 1)] = 0;                  \
  }

uint8_t log2i(unsigned x);

void srand(int seed);
int rand();
int rand_int(int min, int max);

uint64_t fasthash64(const void* m_buf, size_t len, uint64_t seed);

void* memdup(void* buffer, size_t size);
} // namespace scl

#endif
