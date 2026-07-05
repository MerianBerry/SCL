/* hash.cpp
 * scl hash functions
 */

#include "internal.hpp"

namespace scl {
namespace internal {
static uint64_t fasthash64_mix(uint64_t h) {
  h ^= h >> 23;
  h *= 0x2127599bf4325c37ULL;
  h ^= h >> 47;
  return h;
}

uint64_t fasthash64(const void* m_buf, size_t len, uint64_t seed) {
  const uint64_t m = 0x880355f21e6d1965ULL;
  const uint64_t* pos = (const uint64_t*)m_buf;
  const uint64_t* end = pos + (len / 8);
  const unsigned char* pos2;
  uint64_t h = seed ^ (len * m);
  uint64_t v;

  while(pos != end) {
    v = *pos++;
    h ^= fasthash64_mix(v);
    h *= m;
  }

  pos2 = (const unsigned char*)pos;
  v = 0;

  switch(len & 7) {
  case 7:
    v ^= (uint64_t)pos2[6] << 48;
  case 6:
    v ^= (uint64_t)pos2[5] << 40;
  case 5:
    v ^= (uint64_t)pos2[4] << 32;
  case 4:
    v ^= (uint64_t)pos2[3] << 24;
  case 3:
    v ^= (uint64_t)pos2[2] << 16;
  case 2:
    v ^= (uint64_t)pos2[1] << 8;
  case 1:
    v ^= (uint64_t)pos2[0];
    h ^= fasthash64_mix(v);
    h *= m;
  }

  return fasthash64_mix(h);
}
} // namespace internal
} // namespace scl
