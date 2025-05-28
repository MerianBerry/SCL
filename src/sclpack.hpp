/*  sclpak.hpp
 *  SCL package manager
 */

#ifndef SCL_PACKAGER_H
#define SCL_PACKAGER_H

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include <thread>

#define SCL_MAX_WORKERS 2
#define SCL_MAX_CHUNKS  4

namespace scl {
namespace pack {
class Collection;

class PackFile : public Memory {
 private:
  friend class Collection;

 public:
  PackFile (bool locked = false);

  PackFile (const PackFile &rhs)            = delete;
  PackFile &operator= (const PackFile &rhs) = delete;

  PackFile (PackFile &&rhs)                     = default;
  PackFile &operator= (PackFile &&rhs) noexcept = default;
  ~PackFile()                                   = default;
};

class PackCache : public PackFile {
 private:
  bool ready;

 public:
};

class Collection {
 private:
  std::mutex  m_camut;
  std::thread m_workers[SCL_MAX_WORKERS];
  path        m_name;
  // pair of {ismodified, file ptr}
  dictionary<std::pair<bool, PackFile *>> m_index;
  dictionary<PackCache>                   m_cache;
  dictionary<PackFile>                    m_activ;

 public:
  PackFile &open (const path &path);
};

class Packager {
 private:
  std::thread     m_workers[SCL_MAX_WORKERS];
  std::mutex      m_cmut;
  dictionary<int> m_entr;

 public:
  explicit Packager (const Packager &)   = delete;
  Packager &operator= (const Packager &) = delete;

  bool create (const path &colpath);
};
} // namespace pack
} // namespace scl

#endif