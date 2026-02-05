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
class PackIndex;
class Packager;
class PackFetchJob;

class PackWaitable : public jobs::waitable {
  friend class PackIndex;
  friend class PackFetchJob;

  scl::stream *m_stream = nullptr;

 public:
  PackWaitable() = default;
  PackWaitable(scl::stream *stream);

  scl::stream &stream() {
    wait();
    return *m_stream;
  }

  scl::stream *operator->() {
    wait();
    return m_stream;
  }
};

/* Structure that details a packager file/index.
  Contains offset and size info of packaged and source content.
  Also contains info for "active" files.
*/
class PackIndex {
  friend class Packager;
  friend class PackFetchJob;

 private:
  PackWaitable m_wt;
  uint32_t     m_off = 0, m_size = 0, m_original = 0;
  bool         m_active = 0, m_submitted = 0;
  uint8_t      m_pack = 0;

 public:
  PackIndex() = default;

  /**
   * @brief Returns this file's waitable.
   *  Note: The waitable's stream is NULL if
   * this file is inactive.
   *
   * @return waitable reference
   */
  PackWaitable &waitable();

  scl::stream  *operator->() {
    return m_wt.m_stream;
  }

  /**
   * @brief Returns whether or not this file is active (loaded).
   *
   * @return true if active
   * @return false if not
   */
  bool isactive() const;

  /**
   * @brief If this file is active, and hasnt been modified, it will be
   * deactivated (deloaded).
   *
   */
  void release();
};

// Job to decompress a file from a stream into memory
class PackFetchJob : public jobs::job<PackWaitable> {
  PackIndex          &m_indx;
  scl::path           m_file;
  Packager           *m_pack;
  scl::reduce_stream *m_archive;
  scl::stream        *m_out;
  int                 m_sid;

 public:
  PackFetchJob(Packager *pack, scl::reduce_stream *archive, scl::stream *out,
    PackIndex &indx, scl::path file, int sid);

  PackWaitable *getWaitable() const override;

  bool          checkJob(const jobs::JobWorker &worker) const override;

  void          doJob(PackWaitable *wt, const jobs::JobWorker &worker) override;
};

/**
 * @brief Class representing a family of asset packages.
 *  Allows you to read, write, etc.
 *
 */
class Packager : protected std::mutex {
  friend class PackFetchJob;

 private:
  scl::path                  m_family;
  scl::path                  m_ext;
  scl::dictionary<PackIndex> m_index;
  std::vector<PackIndex *>   m_submitted;
  std::atomic_uint32_t       m_waiting;
  bool                       m_open = false;

  enum mPackRes {
    // Continue
    OK = 0,
    // Current element overflowed member pack. Attempt on new member
    OVERFLOW = 1,
    // Error
    ERROR = 2,
  };

  bool     readIndex(scl::reduce_stream &archive);
  mPackRes writeMemberPack(scl::reduce_stream &archive, size_t &elemid,
    int &memberid);

 public:
  Packager();
  ~Packager();

  bool                        open(const scl::path &path);
  PackWaitable               &openFile(const scl::path &path);
  std::vector<PackWaitable *> openFiles(const std::vector<scl::path> &files);
  /**
   * @brief Submits a file to be written to the pack when write() is called.
   *  Note: Files will only be submitted if they have been opened, and are
   * active. (see openFile/openFiles).

   * This function will block simultanious calls.
   *
   * @param  path
   * @return <b>true</b> if file was successfully submitted.
   */
  bool                        submit(const scl::path &path);

  /**
   * @brief Writes all submitted files to the pack.
   * Invalidates all inactive pack files at the time of the call.
   *
   * @param  unload  If true, unloads (deactivates) all entries.
   * @return
   * @return
   */
  bool                        write(bool unload = false);

  const scl::dictionary<PackIndex> &index();

  void                              close();
};

bool packInit();
void packTerminate();
} // namespace pack
} // namespace scl

#endif
