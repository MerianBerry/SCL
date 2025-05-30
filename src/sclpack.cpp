/*  sclpak.cpp
 *  SCL package manager
 */

#include "sclpack.hpp"
#define LZ4_STATIC_LINKING_ONLY
#include "lz4/lz4.h"

namespace scl {
namespace pack {
jobs::JobServer g_serv;

PackWaitable::PackWaitable (stream *cache, stream *active) {
  m_cache  = cache;
  m_active = active;
}

PackWaitable *Packager::open (const path &path) {
  auto ii = m_index[path];
  if (ii != m_index.end()) {
    // File is indexed, but need to find out if it is active, cached, or
    // neither.
    throw "not implemented :)";
  } else {
    m_activ[path] = stream();
    stream *mem   = &m_activ[path].value();
    m_index[path] = {true, mem};
    auto *wt      = new PackWaitable (nullptr, mem);
    wt->complete();
    return wt;
  }
}
} // namespace pack
} // namespace scl