/*  scldict.hpp
 *  SCL dictionary structure
 */

#ifndef SCL_DICT_H
#define SCL_DICT_H

#include "sclcore.hpp"
#include <type_traits>

#ifndef SCL_DICT_MIN
#  define SCL_DICT_MIN 2
#endif

namespace scl {
namespace internal {
template <class T, class K>
struct hnode {
  hnode   *m_next;
  K        m_key;
  T        m_data;
  unsigned m_hash;
};
template <class T, class K, bool NC = true>
class htab_iterator;
} // namespace internal

template <class T, class K = string, class Hfunc = string>
class dictionary : internal::RefObj {
  friend class internal::htab_iterator<T, K>;
  friend class internal::htab_iterator<T, K, false>;

 protected:
  using hnode               = internal::hnode<T, K>;
  using htab_iterator       = internal::htab_iterator<T, K, true>;
  using const_htab_iterator = internal::htab_iterator<T, K, false>;

  uchar           m_hsz;
  unsigned        m_hnum;
  hnode         **m_ht;

  static unsigned ghash(const K &key) {
    return Hfunc::hash(key);
  }

  bool isoptimal() const {
    bool strained = capacity() >= 0.5;
    bool big      = capacity() < 0.25 && m_hsz > SCL_DICT_MIN;
    return !strained && !big && m_hsz >= SCL_DICT_MIN;
  }

  uchar optimal() const {
    return std::max(log2i(m_hnum) + 2, SCL_DICT_MIN);
  }

  unsigned nodei(unsigned hash) const {
    return hash % (1 << m_hsz);
  }

  hnode *gnodebase(unsigned hash) const {
    if(!m_ht || !hash)
      return nullptr;
    return m_ht[nodei(hash)];
  }

  hnode *gnodefull(unsigned hash) const {
    hnode *n = gnodebase(hash);
    while(n && n->m_hash != hash && n->m_next)
      n = n->m_next;
    if(n && n->m_hash == hash)
      return n;
    return nullptr;
  }

  hnode *first() const {
    for(unsigned i = 0; i < 1u << m_hsz && m_ht; i++) {
      if(m_ht[i]) {
        return m_ht[i];
      }
    }
    return nullptr;
  }

  hnode *nodenext(hnode *node) const {
    if(!node)
      return first();
    // If there is another node in the chain
    if(node->m_next)
      return node->m_next;
    // Continue on from the next node in the htab array
    for(unsigned i = nodei(node->m_hash) + 1; i < 1u << m_hsz; i++)
      if(m_ht[i])
        return m_ht[i];
    // No next node could be found
    return nullptr;
  }

  void mutate(bool free) override {
    dictionary fun;
    for(hnode *n = nodenext(nullptr); n;) {
      fun.set(n->m_key, n->m_data);
      n = nodenext(n);
    }
    if(free)
      _clear();
    *this = fun;
  }

  void put(hnode *node) {
    node->m_next = nullptr;
    make_unique();
    hnode *n = gnodebase(node->m_hash);
    while(n && n->m_next && n->m_hash != node->m_hash)
      n = n->m_next;
    m_hnum++;
    // New node in the array
    if(!n)
      m_ht[nodei(node->m_hash)] = node;
    // Replace existing nodes data
    else if(n->m_hash == node->m_hash) {
      m_hnum--;
      n->m_data = std::move(node->m_data);
      delete node;
    } else
      // Chain node
      n->m_next = node;
  }

  void rehash(hnode **oh, unsigned ohsz) {
    for(unsigned i = 0; i < ((unsigned)1 << ohsz); i++) {
      hnode *n = oh[i];
      for(; n;) {
        hnode *m_next = n->m_next;
        put(n);
        n = m_next;
      }
      oh[i] = nullptr;
    }
  }

  void optimize() {
    hnode       **oh   = m_ht;
    unsigned char ohsz = m_hsz;
    m_hsz              = optimal();
    unsigned s         = (1 << m_hsz);
    m_ht               = new hnode *[s];
    if(!m_ht)
      throw "out of memory";
    memset(m_ht, 0, sizeof(hnode *) * s);
    m_hnum = 0;
    if(oh)
      rehash(oh, ohsz), free((void *)oh);
  }

  void _clear() {
    hnode *h = nodenext(nullptr);
    for(; h;) {
      hnode *next = nodenext(h);
      delete h;
      if(next)
        h = next;
      else
        h = 0;
    }
    delete[] m_ht;
    m_ht   = nullptr;
    m_hsz  = 0;
    m_hnum = 0;
  }

 public:
  dictionary() {
    m_hsz  = 0;
    m_hnum = 0;
    m_ht   = nullptr;
  }

  dictionary(const dictionary &rhs)
      : RefObj(rhs), m_ht(rhs.m_ht), m_hnum(rhs.m_hnum), m_hsz(rhs.m_hsz) {
  }

  dictionary &operator=(const dictionary &rhs) {
    if(this->internal::RefObj::operator==(rhs))
      return *this;
    clear();
    ref(rhs);
    m_hnum = rhs.m_hnum;
    m_hsz  = rhs.m_hsz;
    m_ht   = rhs.m_ht;
    return *this;
  }

  ~dictionary() override {
    if(deref() && m_ht)
      _clear();
  }

  /**
   * @brief Clears this dictionary, resetting it.
   *
   */
  void clear() {
    make_unique(false);
    _clear();
  }

  /**
   * @return   Number of elements currently stored
   */
  size_t size() const {
    return m_hnum;
  }

  /**
   * @return   Ratio of usage vs. capacity. 0 is empty, 1 is full.
   */
  double capacity() const {
    return (double)m_hnum / (1ll << m_hsz);
  }

  /**
   * @brief Adds or overwrites an element at a given key.
   *
   * @param key  Key of the element to write to.
   * @param v  Value to associate with that key.
   */
  template <class X                                           = T,
    std::enable_if_t<std::is_copy_assignable<X>::value, bool> = true>
  void set(const K &key, const T &v) {
    // Make this unique
    make_unique();
    if(!isoptimal())
      optimize();
    hnode *node = new hnode();
    if(!node)
      throw "out of memory";
    node->m_key  = key;
    node->m_data = v;
    node->m_hash = ghash(key);
    node->m_next = nullptr;
    put(node);
  }

  /**
   * @brief Adds or overwrites an element at a given key.
   *
   * @param key  Key of the element to write to.
   * @param v  Value to associate with that key.
   */
  template <class X = T, std::enable_if_t<!std::is_copy_assignable<X>::value &&
                                            std::is_move_assignable<X>::value,
                           bool> = true>
  void set(const K &key, T &v) {
    // Make this unique
    make_unique();
    if(!isoptimal())
      optimize();
    hnode *node = new hnode();
    if(!node)
      throw "out of memory";
    node->m_key  = key;
    node->m_data = std::move(v);
    node->m_hash = ghash(key);
    node->m_next = nullptr;
    put(node);
  }

  /**
   * @brief Removes the element at a given key. Does nothing if no such
   * element exists.
   *
   * @param key  Key of the element to remove.
   */
  void remove(const K &key) {
    unsigned hash = ghash(key);
    hnode   *n    = gnodebase(hash);
    while(n && n->m_next && n->m_hash != hash)
      n = n->m_next;
dict_remove_begin:
    if(!n || n->m_hash != hash)
      return;
    if(n && n->m_hash == hash) {
      if(m_ht[nodei(hash)] == n)
        m_ht[nodei(hash)] = n->m_next;
      delete n;
      m_hnum--;
      if(!isoptimal())
        optimize();
    } else if(n->m_next && n->m_next->m_hash == hash) {
      hnode *prev  = n;
      n            = n->m_next;
      prev->m_next = n->m_next;
      goto dict_remove_begin;
    }
  }

  htab_iterator begin() {
    auto *n = first();
    if(n)
      return htab_iterator(*this, n->m_hash, n->m_key);
    else
      return htab_iterator(*this, 0);
  }

  const_htab_iterator cbegin() const {
    auto *n = first();
    if(n)
      return const_htab_iterator(*this, n->m_hash, n->m_key);
    else
      return const_htab_iterator(*this, 0);
  }

  htab_iterator end() {
    return htab_iterator(*this, 0);
  }

  const_htab_iterator cend() const {
    return const_htab_iterator(*this, 0);
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  htab_iterator get(const K &key) {
    unsigned hash = ghash(key);
    hnode   *n    = gnodefull(hash);
    if(n)
      return htab_iterator(*this, n->m_hash, key);
    return end();
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  const_htab_iterator get(const K &key) const {
    unsigned hash = ghash(key);
    hnode   *n    = gnodefull(hash);
    if(n)
      return const_htab_iterator(*this, n->m_hash, key);
    return cend();
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  htab_iterator operator[](const K &key) {
    htab_iterator i = get(key);
    if(i != end())
      return i;
    return htab_iterator(*this, 0, key);
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  const_htab_iterator operator[](const K &key) const {
    const_htab_iterator i = get(key);
    if(i != cend())
      return i;
    return const_htab_iterator(*this, 0, key);
  }

  bool operator==(const dictionary &rhs) const {
    for(const_htab_iterator i = cbegin(), j = rhs.cbegin(); i != cend();
        ++i, ++j) {
      if(j == rhs.cend() || i != j || i.value() != j.value()) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const dictionary &rhs) const {
    return !(*this == rhs);
  }
};

namespace internal {
template <class T, class K, bool NC>
class htab_iterator {
  dictionary<T, K> *m_dict = nullptr;
  K                 m_ikey;
  unsigned          m_hash = 0;

 public:
  htab_iterator() = default;

  htab_iterator(const dictionary<T, K> &dict, unsigned hash)
      : m_dict((dictionary<T, K> *)&dict), m_hash(hash) {
  }

  htab_iterator(const dictionary<T, K> &dict, unsigned hash, const K &key)
      : m_dict((dictionary<T, K> *)&dict), m_hash(hash), m_ikey(key) {
  }

  bool operator==(const htab_iterator &rhs) const {
    return m_hash == rhs.m_hash;
  }

  bool operator!=(const htab_iterator &rhs) const {
    return m_hash != rhs.m_hash;
  }

  htab_iterator &operator++() {
    if(!m_dict)
      return *this = htab_iterator(), *this;
    auto *n = m_dict->nodenext(m_dict->gnodefull(m_hash));
    m_hash  = n ? n->m_hash : 0;
    return *this;
  }

  /* Read */
  /**
   * @brief Returns the key pointed to by this iterator.
   * @exception std::out_of_range.
   *
   * @return   The key of this iterator.
   */
  const K &key() const {
    auto *node = m_dict->gnodefull(m_hash);
    if(!node)
      throw std::out_of_range("");
    return node->m_key;
  }

  /**
   * @brief Returns the value pointed to by this iterator.
   * @exception std::out_of_range.
   *
   * @return   The value of this iterator.
   */
  T &value() const {
    if(!m_dict)
      throw std::out_of_range("");
    auto *node = m_dict->gnodefull(m_hash);
    if(!node)
      throw std::out_of_range("");
    return node->m_data;
  }

  T *operator->() const {
    return &value();
  }

  operator T &() const {
    return value();
  }

  const htab_iterator &operator*() const {
    if(!m_dict)
      throw std::out_of_range("");
    return *this;
  }

  htab_iterator &operator*() {
    return *this;
  }

  template <class X                                                 = T,
    std::enable_if_t<std::is_copy_assignable<X>::value && NC, bool> = true>
  htab_iterator &operator=(const T &val) {
    if(!m_dict)
      throw std::out_of_range("");
    m_dict->make_unique();
    m_hash     = *this == m_dict->end() ? m_dict->ghash(m_ikey) : m_hash;
    auto *node = m_dict->gnodefull(m_hash);
    if(!node)
      m_dict->set(m_ikey, val);
    else
      node->m_data = val;
    return *this;
  }

  template <class X = T,
    std::enable_if_t<!std::is_copy_assignable<X>::value &&
                       std::is_move_assignable<X>::value && NC,
      bool>         = true>
  htab_iterator &operator=(T &&val) {
    if(!m_dict)
      throw std::out_of_range("");
    m_dict->make_unique();
    m_hash     = *this == m_dict->end() ? m_dict->ghash(m_ikey) : m_hash;
    auto *node = m_dict->gnodefull(m_hash);
    if(!node)
      m_dict->set(m_ikey, val);
    else
      node->m_data = std::move(val);
    return *this;
  }
};
} // namespace internal
} // namespace scl

#endif
