/*  scldict.hpp
 *  SCL dictionary structure
 */

#ifndef SCL_DICT_H
#define SCL_DICT_H

#include "sclcore.hpp"

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
template <class T, class K>
class htab_iterator;
} // namespace internal

template <class T, class K = string, class Hfunc = string>
class dictionary : internal::RefObj {
  friend class internal::htab_iterator<T, K>;

 protected:
  using hnode         = internal::hnode<T, K>;
  using htab_iterator = internal::htab_iterator<T, K>;

  uchar    m_hsz;
  unsigned m_hnum;
  hnode  **m_ht;

  static unsigned ghash (K const &key) {
    return Hfunc::hash (key);
  }

  bool isoptimal() const {
    bool nstrained = m_hnum > (1 << (m_hsz - 1));
    bool nbig      = m_hnum > (m_hsz >> 3) && m_hsz > SCL_DICT_MIN;
    return !nstrained && !nbig && m_hsz >= SCL_DICT_MIN;
  }

  uchar optimal() const {
    return std::max (log2i (m_hnum) + 2, SCL_DICT_MIN);
  }

  unsigned nodei (unsigned hash) const {
    return hash % (1 << m_hsz);
  }

  hnode *gnodebase (unsigned hash) const {
    if (!m_ht || !hash)
      return nullptr;
    return m_ht[nodei (hash)];
  }

  hnode *gnodefull (unsigned hash) const {
    hnode *n = gnodebase (hash);
    while (n && n->m_hash != hash && n->m_next)
      n = n->m_next;
    if (n && n->m_hash == hash)
      return n;
    return nullptr;
  }

  hnode *first() const {
    for (unsigned i = 0; i < 1 << m_hsz; i++) {
      hnode *n = m_ht[i];
      if (m_ht[i])
        return m_ht[i];
    }
    return nullptr;
  }

  hnode *nodenext (hnode *node) const {
    if (!node)
      return first();
    // If there is another node in the chain
    if (node->m_next)
      return node->m_next;
    // Continue on from the next node in the htab array
    for (unsigned i = nodei (node->m_hash) + 1; i < 1 << m_hsz; i++)
      if (m_ht[i])
        return m_ht[i];
    // No next node could be found
    return nullptr;
  }

  void mutate (bool free) override {
    dictionary fun;
    for (hnode *n = nodenext (nullptr); n;) {
      fun.set (n->m_key, n->m_data);
      n = nodenext (n);
    }
    if (free)
      _clear();
    *this = fun;
  }

  void put (hnode *node) {
    node->m_next = nullptr;
    make_unique();
    hnode *n = gnodebase (node->m_hash);
    while (n && n->m_next && n->m_hash != node->m_hash)
      n = n->m_next;
    m_hnum++;
    // New node in the array
    if (!n)
      m_ht[nodei (node->m_hash)] = node;
    // Replace existing nodes data
    else if (n->m_hash == node->m_hash) {
      n->m_data = node->m_data;
      delete node;
    } else
      // Chain node
      n->m_next = node;
  }

  void rehash (hnode **oh, unsigned ohsz) {
    for (unsigned i = 0; i < ((unsigned)1 << ohsz); i++) {
      hnode *n = oh[i];
      for (; n;) {
        hnode *m_next = n->m_next;
        put (n);
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
    if (!m_ht)
      throw "out of memory";
    memset (m_ht, 0, sizeof (hnode *) * s);
    m_hnum = 0;
    if (oh)
      rehash (oh, ohsz), free ((void *)oh);
  }

  void _clear() {
    hnode *h = nodenext (nullptr);
    for (; h;) {
      hnode *next = nodenext (h);
      delete h;
      if (next)
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

  dictionary (dictionary const &rhs)
      : RefObj (rhs), m_ht (rhs.m_ht), m_hnum (rhs.m_hnum), m_hsz (rhs.m_hsz) {
  }

  ~dictionary() override {
    if (deref() && m_ht)
      _clear();
  }

  /**
   * @brief Clears this dictionary, resetting it.
   *
   */
  void clear() {
    make_unique (false);
    _clear();
  }

  /**
   * @brief Adds or overwrites an element at a given key.
   *
   * @param key  Key of the element to write to.
   * @param v  Value to associate with that key.
   */
  void set (K const &key, T const &v) {
    // Make this unique
    make_unique();
    if (!isoptimal())
      optimize();
    hnode *node = new hnode();
    if (!node)
      throw "out of memory";
    node->m_key  = key;
    node->m_data = v;
    node->m_hash = ghash (key);
    node->m_next = nullptr;
    put (node);
  }

  /**
   * @brief Removes the element at a given key. Does nothing if no such
   * element exists.
   *
   * @param key  Key of the element to remove.
   */
  void remove (K const &key) {
    unsigned hash = ghash (key);
    hnode   *n    = gnodebase (hash);
    while (n && n->m_next && n->m_next->m_hash != hash)
      n = n->m_next;
    if (n->m_next && n->m_next->m_hash == hash) {
      hnode *node = n->m_next;
      n->m_next   = node->m_next;
      delete node;
      return;
    } else if (n->m_hash == hash) {
      unsigned i = nodei (hash);
      m_ht[i]    = n->m_next;
      delete n;
    }
  }

  htab_iterator begin() {
    return htab_iterator (nodenext (nullptr), *this);
  }

  htab_iterator end() {
    return htab_iterator (nullptr, *this);
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  htab_iterator get (K const &key) {
    unsigned hash = ghash (key);
    hnode   *n    = gnodefull (hash);
    if (n)
      return htab_iterator (n, *this);
    return end();
  }

  /**
   * @brief Returns an iterator to the element at a given key.
   *
   * @param key  Key of the element to search for.
   * @return  Returns an iterator to the specified element. Returns an iterator
   * to dictionary::end() if no such element exists.
   */
  htab_iterator operator[] (K const &key) {
    htab_iterator i = get (key);
    if (i != end())
      return i;
    return htab_iterator (key, *this);
  }
};

namespace internal {
template <class T, class K>
class htab_iterator {
  using hnode = internal::hnode<T, K>;
  hnode            *node;
  K const          *k;
  dictionary<T, K> *dict;

  K const *gkey() const {
    return node ? &node->m_key : k;
  }

 public:
  htab_iterator() : node (nullptr), dict (nullptr) {
  }

  htab_iterator (hnode *node, dictionary<T, K> &dict)
      : node (node), k (nullptr), dict (&dict) {
  }

  htab_iterator (K const &k, dictionary<T, K> &dict)
      : node (nullptr), k (&k), dict (&dict) {
  }

  bool operator== (htab_iterator const &rhs) const {
    return gkey() == rhs.gkey();
  }

  bool operator!= (htab_iterator const &rhs) const {
    return gkey() != rhs.gkey();
  }

  htab_iterator &operator++() {
    if (!dict)
      return node = nullptr, *this;
    node = dict->nodenext (node);
    return *this;
  }

  /* Read */
  /**
   * @brief Returns the key pointed to by this iterator.
   * @exception std::out_of_range The key of this iterator is NULL.
   *
   * @return   The key of this iterator.
   */
  K const &key() const {
    K const *k = gkey();
    if (!k)
      throw std::out_of_range ("Null key");
    return *k;
  }

  /**
   * @brief Returns the value pointed to by this iterator.
   * @exception std::out_of_range No element exists.
   *
   * @return   The value of this iterator.
   */
  T const &value() const {
    if (!node)
      throw std::out_of_range ("No element exists");
    return node->m_data;
  }

  operator T const &() const {
    if (!node || !dict)
      throw std::out_of_range ("Null key or dictionary");
    return node->m_data;
  }

  htab_iterator const &operator*() const {
    if (!gkey() || !dict)
      throw std::out_of_range ("Null key or dictionary");
    return *this;
  }

  htab_iterator &operator*() {
    if (!gkey() || !dict)
      throw std::out_of_range ("Null key or dictionary");
    dict->make_unique();
    K key = *gkey();
    node  = dict->get (key).node;
    return *this;
  }

  htab_iterator &operator= (T const &val) {
    if (!k || !dict)
      throw std::out_of_range ("Null key or dictionary");
    dict->set (*k, val);
    node = dict->gnodefull (dict->ghash (*k));
    return *this;
  }
};
} // namespace internal
} // namespace scl

#endif