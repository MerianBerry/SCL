/*  sclpak.cpp
 *  SCL package manager
 */

#include "sclpack.hpp"
#include "sclxml.hpp"
#define LZ4_STATIC_LINKING_ONLY
#include "lz4/lz4.h"

namespace scl {
namespace pack {
jobs::JobServer g_serv;

PackWaitable::PackWaitable (stream *cache, stream *active) {
  m_cache  = cache;
  m_active = active;
}

bool Packager::load() {
  return true;
}

bool Packager::open (const scl::path &dir, const scl::string &familyName) {
  return true;
}

PackWaitable *Packager::openFile (const path &path) {
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

bool Packager::write() {
  const char maversion = 1;
  const char miversion = 0;
  stream     arcv;
  arcv.open ("archive.sca", true, true);
  char header[32];
  memset (header, 0, sizeof (header));
  memcpy (header, "\x7fSPK", 4);
  memcpy (header + 4, &maversion, 1);
  memcpy (header + 5, &miversion, 1);
  arcv.write (header, sizeof (header));
  const auto boff = sizeof (header);

  xml::XmlDocument doc;
  doc.set_tag (doc, "SCA");
  for (const auto &i : m_index) {
    stream *stream = i->second;
    if (stream) {
      stream->seek (StreamPos::start, 0);
      auto off = string::fmt ("%lli", arcv.tell());
      arcv.write (*stream);
      auto          l = string::fmt ("%lli", stream->tell());
      xml::XmlElem *e = doc.new_elem ("file");
      e->add_attr (doc.new_attr ("name", i.key()));
      e->add_attr (doc.new_attr ("off", off));
      e->add_attr (doc.new_attr ("size", l));
      doc.add_child (e);
    }
  }
  const auto hoff = arcv.tell();
  arcv.seek (StreamPos::start, 8);
  arcv.write (&hoff, sizeof (hoff));

  arcv.seek (StreamPos::start, hoff);
  doc.print (arcv, false);
  return true;
}
} // namespace pack
} // namespace scl