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

#define SCL_MAX_CHUNKS 4

namespace scl {
namespace pack {
class Packager;
class PackFetchJob;

struct PackIndex {
  scl::path m_file;
  size_t    m_off = 0, m_size = 0, m_original = 0;
};

class PackWaitable : public jobs::waitable {
  friend class PackFetchJob;

  scl::stream *m_active;

 public:
  PackWaitable(scl::stream *active);

  scl::stream &content() {
    wait();
    return *m_active;
  }

  scl::stream *operator->() {
    wait();
    return m_active;
  }
};

// Job to decompress a file from a stream into memory
class PackFetchJob : public jobs::job<PackWaitable> {
  PackIndex           m_indx;
  Packager           *m_pack;
  scl::reduce_stream *m_archive;
  scl::stream        *m_out;
  int                 m_sid;

 public:
  PackFetchJob(Packager *pack, scl::reduce_stream *archive, scl::stream *out,
    PackIndex indx, int sid);

  PackWaitable *getWaitable() const override;

  bool          checkJob(const jobs::JobWorker &worker) const override;

  void          doJob(PackWaitable *wt, const jobs::JobWorker &worker) override;
};

enum PackResult {
  OK,
  ERROR,
  FILE,
  FULL,
};

class Packager : protected std::mutex {
  friend class PackFetchJob;

 private:
  scl::path                         m_family;
  scl::path                         m_dir;
  scl::reduce_stream                m_archive;
  scl::dictionary<PackIndex>        m_index;
  // Uncompressed files, user side
  scl::dictionary<scl::stream>      m_activ;
  scl::dictionary<jobs::waitable *> m_wts;
  size_t                            m_ioff = 0;

 public:
  ~Packager();

  bool                        open(const scl::path &path);
  PackWaitable               &openFile(const scl::path &path);
  std::vector<PackWaitable *> openFiles(const std::vector<scl::path> &files);

  const scl::dictionary<PackIndex> &index();

  void                              close();

  bool                              write();
};

bool packInit();
void packTerminate();
} // namespace pack
} // namespace scl

#endif
