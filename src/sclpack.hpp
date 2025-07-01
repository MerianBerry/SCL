/*  sclpak.hpp
 *  SCL package manager
 */

#ifndef SCL_PACKAGER_H
#define SCL_PACKAGER_H

#include "sclcore.hpp"
#include "sclreduce.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include "scljobs.hpp"
#include "sclxml.hpp"

#define SCL_MAX_CHUNKS 4

namespace scl {
namespace pack {
class Packager;
class PackFetchJob;

class PackWaitable : public jobs::waitable {
  friend class PackFetchJob;

  stream *m_cache;
  stream *m_active;

 public:
  PackWaitable(stream *cache, stream *active);

  stream *content() {
    wait();
    return m_active;
  }

  stream *operator->() {
    wait();
    return m_active;
  }
};

class PackFetchJob : public jobs::job<PackWaitable> {
 public:
  PackWaitable *getWaitable() const override;

  void          doJob(PackWaitable *wt, const jobs::JobWorker &worker) override;
};

enum PackResult {
  OK,
  ERROR,
  FILE,
  FULL,
};

class Archive : std::mutex {
  scl::stream      *m_stream;
  xml::XmlDocument *m_manifest;

 public:
};

class PackIndex {
 private:
  scl::stream *m_stream;
  size_t       m_off;
  size_t       m_size;
};

class Packager : protected std::mutex {
  bool load();

 private:
  path                                  m_family;
  path                                  m_dir;
  std::vector<stream *>                 m_streams;
  std::vector<xml::XmlDocument *>       m_mans;
  // pair of {ismodified, file ptr}
  dictionary<std::pair<bool, stream *>> m_index;
  dictionary<stream>                    m_cache;
  dictionary<stream>                    m_activ;

 public:
  bool          open(const scl::path &dir, const scl::string &familyName);
  PackWaitable *openFile(const scl::path &path);

  bool          write();
};
} // namespace pack
} // namespace scl

#endif
