/*  sclxml.hpp
    SCL XML library
*/

#ifndef SCL_XML_H
#define SCL_XML_H

#ifndef SCL_XML_DEFAULT_PRINT_STEP
#  define SCL_XML_DEFAULT_PRINT_STEP 0x2000
#endif

#include "sclcore.hpp"
#include <vector>

namespace scl {
namespace xml {
enum XmlCode {
  OK = 0,
  ERR,
  MEM,
  FILE,
  ALLOC,
  TAG,
  TEXT,
  TEXT_CHILD,
  SPECIAL,
  SYNTAX,
  MISMATCH,
  INCOMPLETE,
  ROOT,
  NIL,
  LEVEL,
  ENUM_MAX,
};

static const scl::string _errdescs[] = {
  "OK",
  "Error (no other info)",
  "Out of memory",
  "Failed to open file",
  "Bad allocation",
  "Bad tag",
  "Bad text",
  "Text node has child",
  "Invalid special character",
  "Syntax error",
  "Beginning/End tag mismatch",
  "Incomplete DOM",
  "Incomplete node",
  "Invalid print level",
  "Invalid root",
};

class XmlResult {
 protected:
  scl::string info;


 public:
  XmlCode code = OK;

  XmlResult()  = default;

  XmlResult(XmlCode code, scl::string info = scl::string())
      : code(code), info(info) {
  }

  /**
   * @return  A brief explenation of this result.
   */
  scl::string what() const {
    switch(code) {
    case TAG:
    case TEXT:
    case SYNTAX:
    case MISMATCH: {
      scl::string out = _errdescs[code];
      if(info)
        out += scl::string::fmt(" (%s)", info.cstr());
      return out;
    }
    default:
      if(code >= 0 && code < ENUM_MAX)
        return _errdescs[code].cstr();
      return "";
    }
  }

  operator bool() const {
    return !code;
  }
};

template <int defaultSize = 2048>
class XmlPage {
 private:
  void    *data = NULL;
  unsigned used = 0;
  unsigned size = 0;
  XmlPage *prev = NULL;
  XmlPage *next = NULL;

 public:
  XmlPage(XmlPage *tonext = NULL) {
    if(tonext) {
      memcpy(this, tonext, sizeof(XmlPage));
      memset(tonext, 0, sizeof(XmlPage));
      tonext->next = this;
    }
  }

  void free() {
    for(XmlPage *page = this; page;) {
      XmlPage *next = page->next;
      if(page->data)
        delete[](char *)page->data;
      // All but the root page must be deleted manually
      if(page != this)
        delete page;
      page = next;
    }
  }

  template <class T = char>
  T *alloc(int n = 1) {
    if(n <= 0)
      return NULL;
    unsigned asz = sizeof(T) * n;
    if(!data) {
      unsigned req = asz > defaultSize ? asz : defaultSize;
      size         = req;
      data         = new char[size];
      if(!data)
        throw XmlResult(MEM);
    }
    if(used + asz > size) {
      /* make a new page, and swap input page for new one */
      /* saves performance and cpu time looking of open slot */
      new XmlPage<defaultSize>(this);
      return alloc<T>(n);
    }
    void *ptr = (char *)data + used;
    used += asz;
    memset(ptr, 0, asz);
    return (T *)ptr;
  }
};

enum XmlFlags {
  none,
  // Skips checking certain XML syntax rules.
  // Using this flag will obfuscate certain error messages.
  no_syntax = 1,
  // Does not check beginning/end tags for elements, and skips checking
  // duplicate attributes.
  // Using this flag will obfuscate certain error messages.
  no_tag_check = 2,
  // Does not replace XML special characters (ex. &quot; => ")
  // If you are sure your document does not use these special characters, this
  // option can dramatically speed up the loading process.
  no_special_expand = 4,
  // Collection of flags that optimize for speed, at the cost of disabling
  // validation.
  // Using this flag will obfuscate most error messages.
  speed_optimze = no_syntax | no_tag_check,
};

enum {
  SPACEBIT = 1,
  ALPHABIT = 2,
  DIGITBIT = 4,
  COLONBIT = 8,
  DELIMBIT = 16,
};

enum XmlPredicate {
  SPACE_PRED = SPACEBIT,
  TAG_PRED   = ALPHABIT | DIGITBIT | COLONBIT,
};

template <class N, class P>
class XmlNode;
class XmlAttr;
class XmlElem;
class XmlDocument;

class XmlAllocator {
  template <class N, class P>
  friend class XmlNode;
  friend class XmlAttr;
  friend class XmlElem;

 protected:
  XmlPage<4096> nodes;
  XmlPage<4096> txt;
};

namespace internal {

/* clang-format off */
  static char constexpr xctypes[] = {
    /* 1 */
    16,0,0,0,0,0,0,0, /* 0-7*/
    0,1,1,0,0,1,0,0, /* 8-15 */
    /* 2 */
    0,0,0,0,0,0,0,0, /* 16-23 */
    0,0,0,0,0,0,0,0, /* 24-31 */
    /* 3 */
    1,0,16,0,0,0,0,0, /* 32-39 */
    0,0,0,0,0,0,0,0, /* 40-47 */
    /* 4 */
    4,4,4,4,4,4,4,4, /* 48-55 */
    4,4,8,0,16,0,0,0, /* 56-63 */
    /* 5 */
    0,2,2,2,2,2,2,2, /* 64-71 */
    2,2,2,2,2,2,2,2, /* 72-79 */
    /* 6 */
    2,2,2,2,2,2,2,2, /* 80-87 */
    2,2,2,0,0,0,0,2, /* 88-95 */
    /* 7 */
    0,2,2,2,2,2,2,2, /* 96-103 */
    2,2,2,2,2,2,2,2, /* 104-111 */
    /* 8 */
    2,2,2,2,2,2,2,2, /* 112-119 */
    2,2,2,0,0,0,0,0, /* 120-127 */

    /* 128-255 */
    2,0,0,0,0,0,0,0, /* 0-7*/
    0,0,0,0,0,0,0,0, /* 8-15 */
    /* 2 */
    0,0,0,0,0,0,0,0, /* 16-23 */
    0,0,0,0,0,0,0,0, /* 24-31 */
    /* 3 */
    0,0,0,0,0,0,0,0, /* 32-39 */
    0,0,0,0,0,0,0,0, /* 40-47 */
    /* 4 */
    0,0,0,0,0,0,0,0, /* 48-55 */
    0,0,0,0,0,0,0,0, /* 56-63 */
    /* 5 */
    0,0,0,0,0,0,0,0, /* 64-71 */
    0,0,0,0,0,0,0,0, /* 72-79 */
    /* 6 */
    0,0,0,0,0,0,0,0, /* 80-87 */
    0,0,0,0,0,0,0,0, /* 88-95 */
    /* 7 */
    0,0,0,0,0,0,0,0, /* 96-103 */
    0,0,0,0,0,0,0,0, /* 104-111 */
    /* 8 */
    0,0,0,0,0,0,0,0, /* 112-119 */
    0,0,0,0,0,0,0,0, /* 120-127 */
  };
/* clang-format on */
} // namespace internal

template <class N, class P>
class XmlNode {
 protected:
  XmlAllocator *m_allo;
  char         *m_tag;
  char         *m_data;
  N            *m_next;

  void          set_allocator(XmlAllocator *allo) {
    m_allo = allo;
  }

  bool skip(XmlPredicate pred, char *s, char **ep) {
    char *p = s;
    while(internal::xctypes[*p] & pred)
      p++;
    return (*ep) = p, s != p;
  }

  void skip_delim(char delim, char *s, char **ep) {
    char *p = s;
    while(*p && *p != delim)
      p++;
    (*ep) = p;
  }

  void expand_text(char *s, char *e) {
    char    *p   = s;
    unsigned cut = 0;
    while(p < (e - cut)) {
      if(*p == '&') {
        s = p++;
        if(!strncmp(p, "lt;", 3))
          (p += 3), (*s = '<');
        else if(!strncmp(p, "gt;", 3))
          (p += 3), (*s = '>');
        else if(!strncmp(p, "amp;", 4))
          (p += 4), (*s = '&');
        else if(!strncmp(p, "apos;", 5))
          (p += 5), (*s = '\'');
        else if(!strncmp(p, "quot;", 5))
          (p += 5), (*s = '\"');
        else
          throw XmlResult(SPECIAL);
        s++;
        cut += (unsigned)(p - s);
        memcpy(s, p, e - p);
        p          = s;
        *(e - cut) = '\0';
      }
      p++;
    }
  }

  template <int f>
  void parse_text(XmlAllocator &allo, char delim, char *s, char **ep) {
    char *p    = s;
    char  hamp = 0;
    char  c;
    while((c = *p) && c != delim) {
      p++;
      // For some reason, this is tanking the parsing speed.
      if(!(f & no_special_expand))
        hamp = hamp || c == '&';
    }
#if 1
    if(hamp)
      expand_text(s, p);
#endif
    if(*p) {
      (*ep)        = p;
      this->m_data = s != p ? s : NULL;
    } else
      throw XmlResult(TEXT);
  }

  template <int step>
  void print_text(stream &stream, const string &t) {
    char *s, *p = (char *)t.cstr();
    s = p;
    for(; *p; p++) {
      switch(*p) {
      case '<':
        stream.write("&lt;", 4, step);
        break;
      case '>':
        stream.write("&gt;", 4, step);
        break;
      case '&':
        stream.write("&amp;", 5, step);
        break;
      case '\'':
        stream.write("&apos;", 6, step);
        break;
      case '\"':
        stream.write("&quot;", 6, step);
        break;
      default:
        continue;
      }
      *p = '\0';
      stream.write((const scl::string &)s, step);
      s = p + 1;
    }
    *p = '\0';
    stream.write((const scl::string &)s, step);
    s = p + 1;
  }

 public:
  XmlNode()                = default;
  XmlNode(const XmlNode &) = delete;

  XmlNode(const scl::string &tag, const scl::string &data = scl::string()) {
    set_tag(tag);
    set_data(data);
  }

  void *operator new(size_t n, XmlAllocator &alloc) {
    XmlNode *ptr = (XmlNode *)alloc.nodes.alloc(sizeof(N));
    ptr->m_allo  = &alloc;
    return ptr;
  }

  void operator delete(void *ptr) = delete;

  /**
   * @return  Pointer to the next node.
   */
  N   *next() const {
    return m_next;
  }

  /**
   * @return  This nodes tag. This string does not own its data. If
   * you need the this value to persist after the owning document is destroyed,
   * use scl::string::copy() on this value.
   */
  scl::string tag() const {
    return m_tag;
  }

  /**
   * @brief Set the tag of the node.
   *
   * @param tag  New tag.
   */
  void set_tag(const scl::string &tag) {
    m_tag = m_allo->txt.alloc((int)tag.len() + 1);
    memcpy(m_tag, tag.cstr(), tag.len());
  }

  /**
   * @brief Set the tag of the node.
   *
   * @param allo  Reference to the document that ownes this node.
   * @param tag  New tag.
   */
  [[deprecated("Use set_tag without the document argument")]] void set_tag(
    XmlAllocator &allo, const scl::string &tag) {
    m_tag = m_allo->txt.alloc((int)tag.len() + 1);
    memcpy(m_tag, tag.cstr(), tag.len());
  }

  /**
   * @return  This nodes data. This string does not own its data. If
   * you need the this value to persist after the owning document is destroyed,
   * use scl::string::copy() on this value.
   */
  scl::string data() const {
    return m_data;
  }

  /**
   * @return   Converts node data into a long long integer if possible.
   */
  long long data_int() const {
    if(m_data)
      return atoll(m_data);
    return 0;
  }

  /**
   * @return   Converts node data into a double if possible.
   */
  double data_float() const {
    if(m_data)
      return atof(m_data);
    return 0.0;
  }

  /**
   * @brief Set the data of the node.
   *
   * @param doc  Reference to the document that ownes this node.
   * @param data  New data.
   */
  void set_data(const string &data) {
    if(data) {
      m_data = m_allo->txt.alloc((int)data.len() + 1);
      memcpy(m_data, data.cstr(), data.len());
    } else {
      m_data = NULL;
    }
  }

  /**
   * @brief Set the data of the node.
   *
   * @param allo  Reference to the document that ownes this node.
   * @param data  New data.
   */
  [[deprecated("Use set_data without the document argument")]] void set_data(
    XmlAllocator &allo, const string &data) {
    if(data) {
      m_data = m_allo->txt.alloc((int)data.len() + 1);
      memcpy(m_data, data.cstr(), data.len());
    } else {
      m_data = NULL;
    }
  }
};

class XmlAttr : public XmlNode<XmlAttr, XmlElem> {
  friend class XmlElem;

 protected:
  template <int f>
  void parse(XmlAllocator &allo, char *s, char **ep) {
    char *p = s;
    if(!skip(TAG_PRED, s, &p))
      throw XmlResult(TAG, s);
    m_tag = s;
    // Check for =" syntax if wanted
    if(!(f & no_syntax)) {
      if(p[0] != '=' && p[1] != '\"')
        throw XmlResult(SYNTAX, p);
    }
    *p         = '\0';
    char delim = p[1];
    s          = p + 2;
    parse_text<f>(allo, delim, s, &p);
    *p    = '\0';
    (*ep) = p + 1;
  }

  template <int s>
  XmlResult print(stream &stream) {
    if(!m_tag)
      throw XmlResult(NIL, "Incomplete attr");
    stream.write(string::fmt("%s=\"", m_tag), s);
    print_text<s>(stream, m_data);
    stream.write("\"", 1, s);
    if(m_next)
      stream.write(" ", 1, s);
    return OK;
  }

 public:
  XmlAttr(const scl::string &tag, const scl::string &data)
      : XmlNode(tag, data) {
  }
};

class XmlElem : public XmlNode<XmlElem, XmlElem> {
 protected:
  XmlElem *m_tail;
  XmlElem *m_child;
  XmlElem *m_parent;
  XmlAttr *m_attr;
  XmlAttr *m_atail;

  void     zero() {
    memset(this, 0, sizeof(XmlElem));
  }

  template <int f>
  void parse_end(int &leave, XmlElem *parent, char *s, char **ep) {
    char *p = s;
    if(!skip(TAG_PRED, s, &p))
      throw XmlResult(TAG, s);
    if(!(f & no_tag_check)) {
      // Check if parent tag and parsed tag are identical
      if(!!strncmp(parent->m_tag, s, p - s)) {
        *p = '\0';
        throw XmlResult(MISMATCH, string::fmt("%s/%s", parent->m_tag, s));
      }
    }
    leave |= 1;
    (*ep) = ++p;
  }

  void parse_pi(char *s, char **ep) {
    // Dont really care
    skip_delim('>', s, ep);
  }

  template <int f>
  void parse(XmlAllocator &allo, XmlElem *parent, char *s, char **ep) {
    static int leave = 0;
    char      *p     = s;
    skip(SPACE_PRED, s, &p);
    if(*p != '<')
      throw XmlResult(INCOMPLETE);
    m_parent = parent;
    s        = ++p;
    if(*p == '/')
      return parse_end<f>(leave, parent, p + 1, ep);
    if(*p == '?') {
      parse_pi(p + 1, &p);
      ++p;
      return parse<f>(allo, parent, p, ep);
    }
    if(!skip(TAG_PRED, s, &p))
      throw XmlResult(TAG, s);
    char *pn = p;
    m_tag    = s;
    skip(SPACE_PRED, p, &p);
    while(*p != '>' && *p != '/' && *p) {
      XmlAttr *attr = allo.nodes.alloc<xml::XmlAttr>();
      attr->parse<f>(allo, p, &p);
      add_attr(attr);
      skip(SPACE_PRED, p, &p);
    }
    if(*p == '>') {
      *pn = '\0';
      s   = ++p;
      skip(SPACE_PRED, p, &p);
      parse_text<f>(allo, '<', p, &p);
      pn = p;
      while(1) {
        xml::XmlElem *celem = allo.nodes.alloc<xml::XmlElem>();
        celem->parse<f>(allo, this, p, &p);
        *pn = '\0';
        if(m_data && !leave)
          throw XmlResult(TEXT_CHILD, m_tag);
        if(leave) {
          leave = 0;
          break;
        }
        add_child(celem);
      }
      (*ep) = p;
      return;
    } else if(*p == '/' || leave) {
      if(!parent)
        throw XmlResult(ROOT);
      p += 1 + (*p == '/');
      leave = 0;
      *pn   = '\0';
      (*ep) = p;
      return;
    }
    throw XmlResult(SYNTAX);
  }

 public:
  XmlElem() {
    zero();
  }

  XmlElem(const scl::string &tag, const scl::string &data = nullptr)
      : XmlNode(tag, data) {
  }

  /**
   * @brief Prints the entire XML tree from this node into `stream`.
   *
   * @tparam s Number of bytes to allocate in out, whenever it must be extended.
   * @param format If true, formats output with new lines and indentation.
   * Defaults to true.
   * @param level Level of the current node. Leave this untouched unless you
   * know what your are doing.
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int s = SCL_XML_DEFAULT_PRINT_STEP>
  XmlResult print(stream &stream, bool format = true, int level = 0) {
    try {
      if(!m_tag)
        throw XmlResult(NIL, "Incomplete elem");
      if(level < 0)
        throw XmlResult(LEVEL, string::fmt("level=%i", level));
      if(level == 0 && format)
        stream.write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", 39, s);
      for(int i = 0; format && i < level; i++)
        stream.write("  ", 2, s);
      stream.write(string::fmt("<%s", m_tag), s);
      if(m_attr) {
        stream.write(" ", 1, s);
        for(auto &a : attributes())
          a->print<s>(stream);
      }
      if(!m_parent || m_data || m_child) {
        stream.write(">", 1, s);
        if(m_data) {
          print_text<s>(stream, m_data);
        } else {
          if(format)
            stream.write("\n", 1, s);
          for(auto &c : children())
            c->print(stream, format, level + 1);
          for(int i = 0; format && i < level; i++)
            stream.write("  ", 2, s);
        }
        stream.write(string::fmt("</%s>", m_tag), s);
        if(m_parent && format)
          stream.write("\n", 1, s);
      } else {
        stream.write("/>", 2, s);
        if(m_parent && format)
          stream.write("\n", 1, s);
      }
    } catch(XmlResult e) {
      return e;
    }
    return OK;
  }

  /**
   * @brief Prints the entire XML tree from this node into `str`.
   *
   * @tparam s Number of bytes to allocate in out, whenever it must be extended.
   * @param format If true, formats output with new lines and indentation.
   * Defaults to true.
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int s = SCL_XML_DEFAULT_PRINT_STEP>
  XmlResult print(scl::string &str, bool format = true) {
    scl::stream stream;
    auto        r = print<s>(stream, format);
    if(!r)
      return r;
    str.claim((const char *)stream.release());
    return OK;
  }

  /**
   * @return  Pointer to this element's first child.
   */
  XmlElem *child() const {
    return m_child;
  }

  /**
   * @return  A vector of this element's children.
   */
  std::vector<XmlElem *> children() const {
    std::vector<XmlElem *> out;
    for(XmlElem *i = m_child; i; i = i->m_next)
      out.push_back(i);
    return out;
  }

  /**
   * @brief Returns a vector of child elements with a tag matching a given name.
   * @param name Tag name to match.
   *
   * @return  A vector of the matching child elements.
   */
  std::vector<XmlElem *> find_children(const scl::string &name) const {
    std::vector<XmlElem *> out;
    for(XmlElem *i = m_child; i; i = i->m_next) {
      if(i->tag() == name)
        out.push_back(i);
    }
    return out;
  }

  /**
   * @return  Number of this element's attributes.
   */
  int num_attrs() const {
    int n = 0;
    for(XmlAttr *i = m_attr; i; i = i->m_next)
      n++;
    return n;
  }

  /**
   * @return  A Vector of this element's attributes.
   */
  std::vector<XmlAttr *> attributes() const {
    std::vector<XmlAttr *> out;
    for(XmlAttr *i = m_attr; i; i = i->m_next)
      out.push_back(i);
    return out;
  }

  /**
   * @brief Returns an attribute with a tag matching the given name, if it
   * exists.
   * @param name Tag name to match.
   *
   * @return  A pointer to the matching attribute. NULL if it doesn't exist.
   */
  XmlAttr *find_attribute(const scl::string &name) const {
    for(XmlAttr *i = m_attr; i; i = i->m_next) {
      if(i->tag() == name)
        return i;
    }
    return nullptr;
  }

  /**
   * @brief  Searches this elements attributes for an attribute with the given
   * name.
   *
   * @param  name  Name of the attribute to find.
   * @return  Pointer to the requested attribute. nullptr if no such attribute
   * exists.
   */
  XmlAttr *find_attr(const scl::string &name) {
    for(XmlAttr *i = m_attr; i; i = i->m_next) {
      if(i->tag() == name)
        return i;
    }
    return nullptr;
  }

  /**
   * @brief  Adds an attribute to this element.
   *
   * @param  attr  Pointer to an attribute to add. See XmlDoc::new_attr().
   */
  void add_attr(XmlAttr *attr) {
    if(!attr)
      throw XmlResult(ALLOC, "Null attr");
    // attr->m_parent = this;
    if(m_attr) {
      if(m_atail)
        m_atail->m_next = attr, m_atail = attr;
      else
        m_attr->m_next = attr, m_atail = attr;
      /*xml::XmlAttr *i = m_attr;
      for (; i && i->m_next; i = i->m_next) {
        // Check for duplicate tag if wanted
      }
      i->m_next = attr;*/
    } else
      m_attr = attr;
  }

  /**
   * @brief  Adds an element as a child to this element.
   *
   * @param  child  Pointer to an element to add. See XmlDoc::new_elem().
   */
  void add_child(XmlElem *child) {
    if(!child)
      throw XmlResult(ALLOC, "Null elem");
    child->m_parent = this;
    if(m_child) {
      xml::XmlElem *elem = m_child;
      if(elem->m_tail)
        elem->m_tail->m_next = child, elem->m_tail = child;
      else
        elem->m_next = child, elem->m_tail = child;
    } else {
      child->m_next = NULL;
      m_child       = child;
    }
  }

  /**
   * @brief  Removes this element from its parent's list of children, if
   * possible.
   *
   */
  void remove() {
    if(!m_parent)
      return;
    xml::XmlElem *el = m_parent->child(), *pel = nullptr;
    for(; el;) {
      if(el == this) {
        if(pel)
          pel->m_next = m_next;
        else
          m_parent->m_child = m_next;
        break;
      }
      pel = el;
      el  = el->m_next;
    }
  }
};

class XmlDocument : public XmlElem, public XmlAllocator {
  friend class XmlElem;

 protected:
  string source;

 public:
  XmlDocument() {
    m_allo = this;
  }

  ~XmlDocument() {
    nodes.free();
    txt.free();
  }

  /**
   * @brief  Loads and parses an XML string into this document.
   * Accepts flags defined in the XmlFlags enum.
   *
   * @tparam  f flags to be passed to the library.
   * @param  content
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int f = none>
  XmlResult load_string(const string &content) {
    this->zero();
    source = content;
    try {
      char *p = (char *)source.cstr();
      if(!p)
        return ERR;
      this->parse<f>(*this, NULL, p, &p);
    } catch(XmlResult e) {
      nodes.free();
      return e;
    }
    return OK;
  }

  /**
   * @brief Loads and parses an XML file into this document.
   * Accepts flags defined in the XmlFlags enum.
   *
   * @tparam  f flags to be passed to the library.
   * @param  path  Path to a file to read from.
   * @param  read  Pointer to a variable, that gets set to the number of bytes
   * read from the file.
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int f = none>
  XmlResult load_file(const string &path, long long *read = NULL) {
    std::ifstream fi(path.cstr());
    if(!fi)
      return FILE;
    fi >> source;
    if(read)
      (*read) += source.size();
    fi.close();
    if(!source)
      return FILE;
    return load_string<f>(source);
  }

  /**
   * @brief  Creates a new attribute, owned by this document.
   *
   */
  XmlAttr *new_attr(const string &tag, const string &data) {
    XmlAttr *a = new(*this) XmlAttr(tag, data);
    return a;
  }

  /**
   * @brief  Creates a new element, owned by this document.
   *
   */
  XmlElem *new_elem(const string &tag, string data = string()) {
    XmlElem *e = new(*this) XmlElem(tag, data);
    return e;
  }
};
} // namespace xml
} // namespace scl

#endif
