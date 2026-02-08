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
class PackWriteJob;

class PackWaitable : public jobs::waitable {
  friend class Packager;
  friend class PackIndex;
  friend class PackFetchJob;
  friend class PackWriteJob;

  int          m_tid    = -1;
  scl::stream* m_stream = nullptr;

 public:
  PackWaitable() = default;
  PackWaitable(scl::stream* stream);

  scl::stream& stream() {
    wait();
    return *m_stream;
  }

  scl::stream* operator->() {
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
  friend class PackWriteJob;

 private:
  PackWaitable m_wt;
  Packager*    m_family;
  scl::string* m_file;
  uint32_t     m_off = 0, m_size = 0, m_original = 0;
  bool         m_active = 0, m_submitted = 0;
  uint8_t      m_pack = 0;

 public:
  PackIndex(const scl::string* file = nullptr);
  PackIndex(PackIndex&& rhs);
  PackIndex&         operator=(PackIndex&& rhs);

  /**
   * @brief Returns the filepath associated with this index
   *
   * @return filepath in the pack
   */
  const scl::string& filepath() const;

  /**
   * @brief Returns the compressed size of this file, if it has been compressed.
   * Returns 0 if it hasnt been compressed.
   * Note: Files are compressed either when they are loaded from a pack, or are
   * written.
   */
  uint32_t           compressed() const {
    return m_size;
  }

  /**
   * @brief Returns the original size of this file, if it is known.
   * Returns 0 if the original size isnt known.
   * Note: Original size is known either when they are loaded from a pack, or
   * are written.
   *
   * @return
   */
  uint32_t original() const {
    return m_original;
  }

  /**
   * @brief Returns this file's waitable.
   *  Note: The waitable's stream is NULL if
   * this file is inactive.
   *
   * @return waitable reference
   */
  PackWaitable& waitable();


  /**
   * @brief Submit this file index for writing.
   * @note If this file index's stream isnt opened by the user, scl will attemt
   * to open it itself, using the given filename, while writing this file index.
   * @note It is reccomended to let scl automatically open this index, if this
   * index represents a local file.
   *
   * @return reference to this object
   */
  PackIndex&    submit();

  /**
   * @brief Convenience function to open this file's stream.
   * Calls scl::stream::open with this index's filepath.
   * For more function info, see scl::stream::open.
   *
   * @param  mode  open mode
   * @param  binary  open in binary mode
   * @return true if opened succesfully
   * @return false if otherwise
   */
  bool          open(OpenMode mode, bool binary = false);

  /**
   * @brief Returns the stream of this file index.
   * @warning This function can return NULL if there is no stream attributed to
   * this index, which can happen if it hasnt been opened via openFile, or
   * openFiles.
   *
   * @return Pointer to this index's stream
   */
  scl::stream*  stream();

  /**
   * @brief Returns the stream of this file index.
   * @warning This function can return NULL if there is no stream attributed to
   * this index, which can happen if it hasnt been opened via openFile, or
   * openFiles.
   *
   * @return Pointer to this index's stream
   */
  scl::stream*  operator->() {
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
   * @brief If this file is active, and isnt submitted, it will be
   * deactivated (deloaded).
   *
   */
  void release();
};

// Job to decompress a file from a stream into memory
class PackFetchJob : public jobs::job<PackWaitable> {
  PackIndex& m_idx;
  Packager&  m_pack;

 public:
  PackFetchJob(PackIndex& idx, Packager& pack);

  PackWaitable* getWaitable() const override;

  bool          checkJob(const jobs::JobWorker& worker) const override;

  void          doJob(PackWaitable* wt, const jobs::JobWorker& worker) override;
};

class PackWriteJob : public jobs::job<PackWaitable> {
  friend class Packager;
  PackIndex& m_idx;
  Packager&  m_pack;

 public:
  PackWriteJob(PackIndex& idx, Packager& pack);

  PackWaitable* getWaitable() const override;

  // bool          checkJob(const jobs::JobWorker& worker) const override;

  void          doJob(PackWaitable* wt, const jobs::JobWorker& worker) override;
};

/**
 * @brief Class representing a family of asset packages.
 *  Allows you to read, write, etc.
 *
 */
class Packager : protected std::mutex {
  friend class PackFetchJob;
  friend class PackWriteJob;
  friend class PackIndex;

 private:
  jobs::JobServer                  m_serv;
  scl::path                        m_family;
  scl::path                        m_ext;
  scl::dictionary<PackIndex>       m_index;
  std::vector<PackIndex*>          m_submitted;
  std::vector<scl::reduce_stream*> m_archives;
  // Reduce queue mutex
  std::mutex                       m_remux;
  std::queue<scl::reduce_stream*>  m_reduces;
  // Queue of in-progress compressions
  std::queue<PackIndex*>           m_writing;
  std::atomic_uint32_t             m_waiting;
  int                              m_workers;
  bool                             m_open = false;

  enum mPackRes {
    // Continue
    OK = 0,
    // Current element overflowed member pack. Attempt on new member
    WOVERFLOW = 1,
    // Error
    ERROR = 2,
  };

  bool     readIndex(scl::reduce_stream& archive, uint32_t bid);
  mPackRes writeMemberPack(scl::stream& archive, size_t& elemid, int memberid,
    const scl::string& buildid, std::function<void(size_t, PackIndex*)>& cb);

 public:
  Packager(int nworkers = INT_MAX);
  ~Packager();

  /**
   * @brief Opens the pack family from the given name
   *
   * @param  path  pack family path
   * @return true if success
   * @return false if otherwise
   */
  bool                    open(const scl::path& path);

  /**
   * @brief Requests the given filepath to be indexed (if not already), or
   * decompressed (if already in pack).
   *
   * @param  path filepath to be opened
   * @return Pointer to the pack index of the given path.
   */
  PackIndex*              openFile(const scl::path& path);

  /**
   * @brief Vectored version of openFile. For info, see openFile().
   * @see openFile
   *
   * @param  files  vector of files to open.
   * @return Vector of pack indices for the given files.
   */
  std::vector<PackIndex*> openFiles(const std::vector<scl::path>& files);

  /**
   * @brief Submits a file to be written to the pack when write() is called.
   *  Note: Files will only be written if they are active by the time write() is
   called.

   * This function will block simultanious calls.
   *
   * @param  path
   * @return <b>true</b> if file was successfully submitted.
   */
  bool                    submit(const scl::path& path);

  /**
   * @brief Writes all submitted files to the pack.
   * Invalidates all inactive pack files at the time of the call.
   * @warning This function will call close(), and invalidate all info related
   * to this packager.
   *
   * @param  cb Callback called with (elementid, pack_index), elementid being
   * its submission id, and pack_index being the index related to it. Called
   * after a file has been succesfully compressed, but not fully written.
   * @return true on success
   * @return false if otherwise.
   */
  bool write(std::function<void(size_t, PackIndex*)> cb = {});

  /**
   * @brief Returns the dictionary containing every index known in this pack
   * family.
   *
   * @return
   */
  const scl::dictionary<PackIndex>& index();

  PackIndex*                        operator[](const scl::string& path);

  /**
   * @brief Closes this pack.
   *
   */
  void                              close();
};

bool packInit();
void packTerminate();
} // namespace pack
} // namespace scl

#endif
