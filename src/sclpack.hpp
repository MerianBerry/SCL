/*  sclpak.hpp
 *  SCL package manager
 */

#ifndef SCL_PACKAGER_H
#define SCL_PACKAGER_H

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include "scljobs.hpp"
#include <thread>

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
  PackWaitable (stream *cache, stream *active);

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

  void doJob (PackWaitable *wt, const jobs::JobWorker &worker) override;
};

class Packager : protected std::mutex {
 private:
  path m_name;
  // pair of {ismodified, file ptr}
  dictionary<std::pair<bool, stream *>> m_index;
  dictionary<stream>                    m_cache;
  dictionary<stream>                    m_activ;

 public:
  PackWaitable *open (const path &path);
};
} // namespace pack
} // namespace scl

#endif