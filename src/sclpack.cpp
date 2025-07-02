/*  sclpak.cpp
 *  SCL package manager
 */

#include "sclpack.hpp"
#include "sclxml.hpp"

#define SPK_HEADER_SIZE 32
#define SPK_MAGIC       "SPK\x7f"
#define SPK_IOFF        8


static scl::jobs::JobServer g_serv;

bool                        is_little_endiann() {
  volatile uint32_t i = 0x01234567;
  // return 0 for big endian, 1 for little endian.
  return (*((uint8_t *)(&i))) == 0x67;
}

namespace scl {
namespace pack {

PackWaitable::PackWaitable(scl::stream *active) {
  m_active = active;
}

PackFetchJob::PackFetchJob(Packager *pack, scl::reduce_stream *archive,
  scl::stream *out, PackIndex indx, int sid) {
  m_indx    = indx;
  m_pack    = pack;
  m_archive = archive;
  m_out     = out;
  m_sid     = sid;
}

PackWaitable *PackFetchJob::getWaitable() const {
  auto *wt = new PackWaitable(m_out);
  // Add the waitable, to the package waitable dicitonary
  m_pack->m_wts[m_indx.m_file] = wt;
  return wt;
}

bool PackFetchJob::checkJob(const jobs::JobWorker &worker) const {
  size_t           bits = 1llu << m_sid;
  jobs::JobServer &serv = worker.serv();
  // Is the target stream locked?
  if(serv.hasLockBits(bits))
    return false;
  // Lock target stream bit
  serv.setLockBits(bits);
  return true;
}

void PackFetchJob::doJob(PackWaitable *wt, const jobs::JobWorker &worker) {
  // Target stream lock bit
  size_t bits = 1llu << m_sid;
  m_archive->seek(StreamPos::start, m_indx.m_off);
  // Begin decompression stream
  if(!m_archive->begin(reduce_stream::Decompress))
    return;
  // Decompress into m_out, with a max byte count of m_indx.m_original
  m_out->reserve(m_indx.m_original);
  if(!m_out->write(*m_archive, m_indx.m_original))
    return;
  // Reset modified status, so later code works properly
  m_out->reset_modified();
  // End decompression stream
  m_archive->end();
  if(m_out->tell() != m_indx.m_original) {
    worker.sync([&]() {
      printf("Warning: Read underflow for %s\n", m_indx.m_file.cstr());
      fflush(stdout);
    });
  }
  // Remove the waitable from the package
  m_pack->m_wts.remove(m_indx.m_file);
  worker.serv().unsetLockBits(bits);
}

Packager::~Packager() {
  close();
}

bool Packager::open(const scl::path &path) {
  scl::stream pack;
  if(!pack.open(path, false, true))
    return false;
  char   header[SPK_HEADER_SIZE];
  size_t read = pack.read(header, sizeof(header));
  if(read < SPK_HEADER_SIZE) {
    // New archive
    pack.seek(StreamPos::start, 0);
    m_archive = std::move(pack);
    return true;
  }
  if(strncmp(header, SPK_MAGIC, sizeof(SPK_MAGIC) - 1) != 0) {
    // Not a spk archive
    return false;
  }
  // Load index offset
  memcpy(&m_ioff, header + SPK_IOFF, sizeof(m_ioff));
  if(m_ioff) {
    pack.seek(StreamPos::start, m_ioff);
    scl::string content;
    pack >> content;
    xml::XmlDocument doc;
    xml::XmlResult   ret = doc.load_string<xml::speed_optimze>(content);
    if(!ret) {
      // TODO: replace with proper logging eventually
      fprintf(stderr, "Failed to index spk archive. reason: %s\n",
        ret.what().cstr());
      return false;
    }
    if(doc.tag() != "SPK") {
      // Malformed
      return false;
    }
    // Load file index
    for(auto c : doc.children()) {
      if(c->tag() == "file") {
        PackIndex indx;
        auto     *name     = c->find_attr("name");
        auto     *off      = c->find_attr("off");
        auto     *size     = c->find_attr("size");
        auto     *original = c->find_attr("original");
        if(!name || !off || !size || !original) {
          // Malformed
          continue;
        }
        indx.m_file          = name->data().copy();
        indx.m_off           = off->data_int();
        indx.m_size          = size->data_int();
        indx.m_original      = original->data_int();
        m_index[indx.m_file] = indx;
      }
    }
  }
  // Skip checking version for now
  m_archive = std::move(pack);
  return true;
}

PackWaitable &Packager::openFile(const path &path) {
  lock();
  auto ia = m_activ[path];
  if(ia != m_activ.end()) {
    // The file is active, so just return that
    auto *wt = new PackWaitable(&ia.value());
    wt->complete();
    unlock();
    return *wt;
  } else {
    // Check if it is indexed, if so, fetch. If not, add new entry to m_activ.
    auto ii = m_index[path];
    // New active entry is added either way.
    ia = scl::stream();
    if(ii != m_index.end()) {
      // File is indexed, fetch it.
      auto &wt = g_serv.submitJob(
        new PackFetchJob(this, &m_archive, &ia.value(), ii.value(), 0));
      unlock();
      return wt;
    } else {
      // New empty active entry, so return active entry
      auto *wt = new PackWaitable(&ia.value());
      wt->complete();
      unlock();
      return *wt;
    }
  }
}

std::vector<PackWaitable *> Packager::openFiles(
  const std::vector<scl::path> &files) {
  std::vector<PackWaitable *> waitables;
  for(auto &i : files) {
    waitables.push_back(&openFile(i));
  }
  return waitables;
}

bool Packager::write() {
  const char maversion = 1;
  const char miversion = 0;

  if(!is_little_endiann()) {
    fprintf(stderr, "SPK only configured for little endian systems\n");
    fflush(stderr);
    throw "SPK only configured for little endian systems";
  }

  if(!m_archive.is_open())
    return false;

  lock();
  if(!m_ioff) {
    // New archive
    char header[SPK_HEADER_SIZE];
    memset(header, 0, sizeof(header));
    memcpy(header, SPK_MAGIC, 4);
    memcpy(header + 4, &maversion, 1);
    memcpy(header + 5, &miversion, 1);
    m_archive.seek(StreamPos::start, 0);
    m_archive.write_uncompressed(header, sizeof(header));
    m_ioff = SPK_HEADER_SIZE;
  }

  size_t aoff = m_ioff;
  m_archive.seek(StreamPos::start, aoff);

  xml::XmlDocument doc;
  doc.set_tag(doc, "SPK");

  for(const auto &i : m_index) {
    auto ia = m_activ[i.key()];
    if(ia != m_activ.end()) {
      if(ia->is_modified())
        continue;
      else
        m_activ.remove(i.key());
    }
    xml::XmlElem *e = doc.new_elem("file");
    e->add_attr(doc.new_attr("name", i.key()));
    e->add_attr(doc.new_attr("off", scl::string::fmt("%lli", i->m_off)));
    e->add_attr(doc.new_attr("size", scl::string::fmt("%lli", i->m_size)));
    e->add_attr(
      doc.new_attr("original", scl::string::fmt("%lli", i->m_original)));
    doc.add_child(e);
  }

  for(const auto &i : m_activ) {
    size_t srcSize = i->seek(StreamPos::end, 0);
    i->seek(StreamPos::start, 0);
    m_archive.begin(reduce_stream::Compress);
    if(!m_archive.write(i.value(), srcSize)) {
      // TODO use an updated logging method
      fprintf(stderr, "Failed to compress file\n");
      m_archive.end();
      m_archive.seek(StreamPos::start, aoff);
      continue;
    }
    m_archive.end();
    size_t        outSize = m_archive.tell() - aoff;

    xml::XmlElem *e       = doc.new_elem("file");
    e->add_attr(doc.new_attr("name", i.key()));
    e->add_attr(doc.new_attr("off", scl::string::fmt("%lli", aoff)));
    e->add_attr(doc.new_attr("size", scl::string::fmt("%lli", outSize)));
    e->add_attr(doc.new_attr("original", scl::string::fmt("%lli", srcSize)));
    doc.add_child(e);

    aoff += outSize;
  }

  m_ioff = aoff;
  m_archive.seek(StreamPos::start, SPK_IOFF);
  m_archive.write_uncompressed(&aoff, sizeof(aoff));

  m_archive.seek(StreamPos::start, aoff);
  scl::stream stream = std::move(m_archive);
  doc.print(stream, false);
  m_archive = std::move(stream);
  unlock();
  return true;
}

const scl::dictionary<PackIndex> &Packager::index() {
  return m_index;
}

void Packager::close() {
  lock();
  // Wait for all the unfinished waitables, so they dont error
  if(m_wts.size()) {
    for(auto i = m_wts.begin(); i != m_wts.end(); i = m_wts.begin()) {
      auto *wt = i.value();
      unlock();
      wt->wait();
      lock();
    }
  }
  m_family = path();
  m_dir    = path();
  m_archive.close();
  m_index.clear();
  m_activ.clear();
  m_ioff = 0;
  unlock();
}

bool packInit() {
  g_serv.slow();
  g_serv.start();
  return true;
}

void packTerminate() {
  g_serv.stop();
}
} // namespace pack
} // namespace scl
