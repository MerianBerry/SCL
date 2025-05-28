/*  sclpak.cpp
 *  SCL package manager
 */

#include "sclpack.hpp"
#define LZ4_STATIC_LINKING_ONLY
#include "lz4/lz4.h"

namespace scl {
namespace pack {
PackFile::PackFile (bool locked) {
  if (locked)
    m_mut.lock();
}

PackFile &Collection::open (const path &path) {
  auto ii = m_index[path];
  if (ii != m_index.end()) {
    // File is indexed, but need to find out if it is active, cached, or
    // neither.
    throw "not implemented :)";
  } else {
    PackFile f;
    m_activ[path] = f;
    PackFile &F   = m_activ[path];
    m_index[path] = {true, &F};
    return F;
  }
}
} // namespace pack
} // namespace scl