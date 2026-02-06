/*  sclpak.cpp
 *  SCL package manager
 */

#include "sclpack.hpp"
#include "sclxml.hpp"
#include <memory>

#define SPK_MAJOR       2
#define SPK_MINOR       0

#define SPK_HEADER_SIZE 32
#define SPK_MAGIC       "SPK\x7f"
#define SPK_H_MAJOR     4
#define SPK_H_MINOR     5
#define SPK_H_MID       6
#define SPK_H_IOFF      8

#define SPK_MAX_MEMBERS 32

#ifndef SPK_MAX_PACK_SIZE
#  define SPK_MAX_PACK_SIZE 0x3fffffff
#endif

using scl::xml::XmlAttr;
using scl::xml::XmlElem;

static scl::jobs::JobServer g_serv;

bool                        is_little_endiann() {
  volatile uint32_t i = 0x01234567;
  // return 0 for big endian, 1 for little endian.
  return (*((uint8_t*)(&i))) == 0x67;
}

namespace scl {
namespace pack {

PackWaitable::PackWaitable(scl::stream* stream) {
  m_stream = stream;
}

PackFetchJob::PackFetchJob(Packager* pack, scl::reduce_stream* archive,
  scl::stream* out, PackIndex& indx, scl::path file, int sid)
    : m_indx(indx), m_file(file) {
  m_pack    = pack;
  m_archive = archive;
  m_out     = out;
  m_sid     = sid;
}

PackIndex::PackIndex(const scl::string* file) : m_file((scl::string*)file) {
}

PackIndex::PackIndex(PackIndex&& rhs) : m_file(rhs.m_file) {
  m_wt        = std::move(rhs.m_wt);
  m_family    = rhs.m_family;
  m_off       = rhs.m_off;
  m_size      = rhs.m_size;
  m_original  = rhs.m_original;
  m_active    = rhs.m_active;
  m_submitted = rhs.m_submitted;
  m_pack      = rhs.m_pack;
}

PackIndex& PackIndex::operator=(PackIndex&& rhs) {
  m_wt        = std::move(rhs.m_wt);
  m_family    = rhs.m_family;
  m_file      = rhs.m_file;
  m_off       = rhs.m_off;
  m_size      = rhs.m_size;
  m_original  = rhs.m_original;
  m_active    = rhs.m_active;
  m_submitted = rhs.m_submitted;
  m_pack      = rhs.m_pack;
  return *this;
}

const scl::string& PackIndex::filepath() const {
  return *m_file;
}

PackWaitable& PackIndex::waitable() {
  return m_wt;
}

PackIndex& PackIndex::submit() {
  if(!m_family)
    return *this;
  m_family->lock();
  m_family->m_submitted.push_back(this);
  m_family->unlock();
  return *this;
}

bool PackIndex::open(OpenMode mode, bool binary) {
  return m_wt->open(*m_file, mode, binary);
}

bool PackIndex::isactive() const {
  return m_active;
}

void PackIndex::release() {
  if(m_active && !m_wt->is_modified()) {
    m_active = false;
    delete &m_wt.stream();
  }
}

PackWaitable* PackFetchJob::getWaitable() const {
  // Add the waitable, to the package waitable dicitonary
  // m_pack->m_wts[m_indx.m_file] = wt;
  m_pack->m_waiting.fetch_add(1);
  return &m_indx.m_wt;
}

bool PackFetchJob::checkJob(const jobs::JobWorker& worker) const {
  size_t           bits = 1llu << m_sid;
  jobs::JobServer& serv = worker.serv();
  // Is the target stream locked?
  if(serv.hasLockBits(bits))
    return false;
  // Lock target stream bit
  serv.setLockBits(bits);
  return true;
}

void PackFetchJob::doJob(PackWaitable* wt, const jobs::JobWorker& worker) {
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
      printf("Warning: Read underflow for %s\n", m_file.cstr());
      fflush(stdout);
    });
  }
  // Remove the waitable from the package
  // m_pack->m_wts.remove(m_indx.m_file);
  m_pack->m_waiting.fetch_sub(1);
  worker.serv().unsetLockBits(bits);
}

Packager::Packager() {
  m_waiting = 0;
}

Packager::~Packager() {
  close();
}

bool Packager::readIndex(scl::stream& archive) {
  scl::string content;
#if 0
  archive >> content;
  xml::XmlDocument doc;
  xml::XmlResult   ret = doc.load_string<xml::speed_optimize>(content);
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
      auto*     name     = c->find_attr("name");
      auto*     off      = c->find_attr("off");
      auto*     size     = c->find_attr("size");
      auto*     original = c->find_attr("original");
      if(!name || !off || !size || !original) {
        // Malformed
        continue;
      }
      indx.m_off            = off->data_int();
      indx.m_size           = size->data_int();
      indx.m_original       = original->data_int();
      m_index[name->data()] = std::move(indx);
    }
  }
#endif
  return true;
}

bool Packager::open(const scl::path& path) {
  m_open   = true;
  m_ext    = path.extension();
  m_family = path;
  m_family.replaceExtension("");
#if 0
  scl::stream pack;
  m_family = path;
  if(!pack.open(path, OpenMode::RWb))
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
  memcpy(&m_ioff, header + SPK_H_IOFF, sizeof(m_ioff));
  if(m_ioff) {
    pack.seek(StreamPos::start, m_ioff);

  }
  // Skip checking version for now
  m_archive = std::move(pack);
  m_open    = true;
#endif
  return true;
}

PackIndex* Packager::openFile(const path& path) {
  // Syncronous, cause it gotta be. (its cheap-ish).
  lock();
  auto idx = m_index[path];
  if(idx == m_index.end()) {
    // File does not exist in index, so make a new active one.
    idx = std::move(PackIndex());
    PackIndex nidx(&idx.key());
    nidx.m_wt = PackWaitable(new scl::stream());
    nidx.m_wt.complete();
    nidx.m_family = this;
    nidx.m_active = true;
    idx           = std::move(nidx);
    unlock();
    return &idx.value();
  } else if(idx->m_active) {
    // Active file. Do nothing.
    unlock();
    return &idx.value();
  } else {
    // File is indexed, but not active.
    auto* stream  = new scl::stream();
    idx->m_wt     = PackWaitable(stream);
    idx->m_active = true;
#if 0
    auto &wt      = g_serv.submitJob(
      new PackFetchJob(this, &m_archive, stream, idx.value(), path, 0));
#endif
    unlock();
    return &idx.value();
  }
}

std::vector<PackIndex*> Packager::openFiles(
  const std::vector<scl::path>& files) {
  std::vector<PackIndex*> indices;
  for(auto& i : files) {
    indices.push_back(openFile(i));
  }
  return indices;
}

bool Packager::submit(const scl::path& path) {
  lock();
  auto idx = m_index[path];
  if(idx != m_index.end()) {
    m_submitted.push_back(&idx.value());
    unlock();
    return true;
  }
  unlock();
  return false;
}

Packager::mPackRes Packager::writeMemberPack(scl::stream& archive,
  size_t& elemid, int memberid, std::function<void(size_t, PackIndex*)>& cb) {
  scl::path outpath;
  if(!memberid)
    outpath = scl::string::fmt("%s%s", m_family.cstr(), m_ext.cstr());
  else
    outpath =
      scl::string::fmt("%s%i%s", m_family.cstr(), memberid - 1, m_ext.cstr());

  archive.open(outpath, OpenMode::RWTRUNC, true);

  // Setup and write pack header
  char header[SPK_HEADER_SIZE];
  memset(header, 0, sizeof(header));
  memcpy(header, SPK_MAGIC, 4);
  header[SPK_H_MAJOR] = SPK_MAJOR;
  header[SPK_H_MINOR] = SPK_MINOR;
  header[SPK_H_MID]   = memberid;
  archive.write(header, SPK_HEADER_SIZE);

  size_t             itabsize = 0;
  scl::stream        itab;
  size_t             off = SPK_HEADER_SIZE;
  scl::reduce_stream reduce;
  mPackRes           res     = OK;
  bool               written = false;
  for(; elemid < m_submitted.size(); elemid++) {
    auto* idx = m_submitted[elemid];
    // Skip if inactive
    if(!idx->m_active)
      continue;
    // Compress file entry
    reduce.begin(reduce_stream::Compress);
    idx->m_wt.stream().seek(StreamPos::end, 0);
    // Estimate compressed size, and reserve the space
    size_t ask = idx->m_wt.stream().tell() / 3 * 2;
    if(reduce.size() < ask)
      reduce.reserve(ask - reduce.size());
    idx->m_wt.stream().seek(StreamPos::start, 0);
    reduce.write(idx->m_wt.stream());
    reduce.end();
    // Update index
    idx->m_off      = off;
    idx->m_size     = reduce.tell();
    idx->m_original = idx->m_wt.stream().tell();
    idx->m_wt.stream().close();
    delete &idx->m_wt.stream();
    idx->m_active = false;
    if(cb)
      cb(elemid, idx);
    reduce.seek(StreamPos::start, 0);
    // itab entry size estimation
    // 2 path length, 4*3 for offset, size, and original size
    uint16_t newitab = 14 + idx->m_file->len();
    // check for pack overflow before writing
    if(off + idx->m_size + itabsize + newitab >= SPK_MAX_PACK_SIZE) {
      // Overflow
      if(written)
        // Overflow so a new member can be made.
        res = WOVERFLOW;
      else {
        // first file to be written causes overflow
        // there is no recourse for this, so error.
        fprintf(stderr, "file %s is too big to be written\n",
          idx->m_file->cstr());
        res = ERROR;
      }
      break;
    }
    // Write compressed content
    archive.write(reduce, idx->m_size);
    uint16_t filelen = idx->m_file->len();
    // Write itab entry;
    itab.write(&filelen, 2, SCL_STREAM_BUF);
    itab.write(*idx->m_file);
    itab.write(&idx->m_off, 4);
    itab.write(&idx->m_size, 4);
    itab.write(&idx->m_original, 4);
    reduce.seek(StreamPos::start, 0);
    // Update info
    off += idx->m_size;
    itabsize += newitab;
    written = true;
  }
  reduce.close();
  // Write itab at the end of the archive
  itab.seek(StreamPos::start, 0);
  archive.write(itab, itabsize);
  // Write itab offset in the archive header
  archive.seek(StreamPos::start, SPK_H_IOFF);
  archive.write(&off, 4);
  archive.close();
  return res;
}

bool Packager::write(std::function<void(size_t, PackIndex*)> cb) {
  const char maversion = 1;
  const char miversion = 0;

  if(!is_little_endiann()) {
    fprintf(stderr, "SPK only configured for little endian systems\n");
    fflush(stderr);
    throw "SPK only configured for little endian systems";
  }

  if(!m_open)
    return false;

  lock();
  scl::stream archive;
  size_t      elem = 0;
  int         mid  = 0;

  while(writeMemberPack(archive, elem, mid, cb) == WOVERFLOW)
    mid++;
  m_submitted.resize(0);
  unlock();
  return true;
}

const scl::dictionary<PackIndex>& Packager::index() {
  return m_index;
}

PackIndex* Packager::operator[](const scl::string& path) {
  auto idx = m_index[path];
  if(idx == m_index.end())
    return nullptr;
  return &idx.value();
}

void Packager::close() {
  lock();
  if(!m_open) {
    unlock();
    return;
  }
  // Wait for all the unfinished waitables, so they dont error
  if(!waitUntil(
       [this]() {
         return m_waiting == 0;
       },
       5, 1)) {
    fprintf(stderr, "Packager::close timed out due to unfinished jobs\n");
  };
  for(auto& i : m_index) {
    if(i->m_active) {
      i->m_wt->close();
      delete &i->m_wt.stream();
    }
  }
  m_family = path();
  m_ext    = string();
  m_index.clear();
  m_submitted.clear();
  m_waiting = 0;
  m_open    = false;
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
