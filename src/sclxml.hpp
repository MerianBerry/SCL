/*  sclxml.hpp
    SCL XML library
*/

#include "sclcore.hpp"
#include <vector>

namespace scl {
namespace xml {
enum xml_excode {
  OK = 0,
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

class xml_result {
 protected:
  string info;

 public:
  xml_excode code;

  xml_result (xml_excode code, string info = string());

  string what() const;
  operator bool() const;
};

template <int defaultSize = 2048>
class xml_page {
 private:
  void     *data = NULL;
  unsigned  used = 0;
  unsigned  size = 0;
  xml_page *prev = NULL;
  xml_page *next = NULL;

 public:
  xml_page (xml_page *tonext = NULL) {
    if (tonext) {
      memcpy (this, tonext, sizeof (xml_page));
      memset (tonext, 0, sizeof (xml_page));
      tonext->next = this;
    }
  }

  void free() {
    for (xml_page *page = this; page;) {
      xml_page *next = page->next;
      if (page->data)
        delete[] (char *)page->data;
      // All but the root page must be deleted manually
      if (page != this)
        delete page;
      page = next;
    }
  }

  template <class T = char>
  T *alloc (int n = 1) {
    if (n <= 0)
      return NULL;
    unsigned asz = sizeof (T) * n;
    if (!data) {
      unsigned req = asz > defaultSize ? asz : defaultSize;
      size         = req;
      data         = new char[size];
      if (!data)
        throw xml_result (MEM);
    }
    if (used + asz > size) {
      /* make a new page, and swap input page for new one */
      /* saves performance and cpu time looking of open slot */
      new xml_page<defaultSize> (this);
      return alloc<T> (n);
    }
    void *ptr = (char *)data + used;
    used += asz;
    memset (ptr, 0, asz);
    return (T *)ptr;
  }
};

#pragma pack(push, 4)

class string_view {
 public:
  char    *s;
  unsigned l;
};

#pragma pack(pop)

enum xml_flags {
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

enum xml_pred {
  SPACE_PRED = SPACEBIT,
  TAG_PRED   = ALPHABIT | DIGITBIT | COLONBIT,
};

template <class N, class P>
class xml_node;
class xml_attr;
class xml_elem;
class xml_doc;

class xml_allocator {
  template <class N, class P>
  friend class xml_node;
  friend class xml_attr;
  friend class xml_elem;

 protected:
  xml_page<4096> nodes;
  xml_page<4096> txt;
};

template <class N, class P>
class xml_node {
 protected:
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

  char *p_tag;
  char *p_data;
  N    *p_next;

  bool skip (xml_pred pred, char *s, char **ep) {
    char *p = s;
    while (xctypes[*p] & pred)
      p++;
    return (*ep) = p, s != p;
  }

  void skip_delim (char delim, char *s, char **ep) {
    char *p = s;
    while (*p && *p != delim)
      p++;
    (*ep) = p;
  }

  void expand_text (char *s, char *e) {
    char    *p   = s;
    unsigned cut = 0;
    while (p < (e - cut)) {
      if (*p == '&') {
        s = p++;
        if (!strncmp (p, "lt;", 3))
          (p += 3), (*s = '<');
        else if (!strncmp (p, "gt;", 3))
          (p += 3), (*s = '>');
        else if (!strncmp (p, "amp;", 4))
          (p += 4), (*s = '&');
        else if (!strncmp (p, "apos;", 5))
          (p += 5), (*s = '\'');
        else if (!strncmp (p, "quot;", 5))
          (p += 5), (*s = '\"');
        else
          throw xml_result (SPECIAL);
        s++;
        cut += (p - s);
        memcpy (s, p, e - p);
        p          = s;
        *(e - cut) = '\0';
      }
      p++;
    }
  }

  template <int f>
  void parse_text (xml_allocator &allo, char delim, char *s, char **ep) {
    char *p    = s;
    char  hamp = 0;
    char  c;
    while ((c = *p) && c != delim) {
      p++;
      // For some reason, this is tanking the parsing speed.
      if (!(f & no_special_expand))
        hamp = hamp || c == '&';
    }
#if 1
    if (hamp)
      expand_text (s, p);
#endif
    if (*p) {
      (*ep)        = p;
      this->p_data = s != p ? s : NULL;
    } else
      throw xml_result (TEXT);
  }

  template <int step>
  void print_text (string &out, string const &t) {
    char *s, *p = (char *)t.cstr();
    s = p;
    for (; *p; p++) {
      switch (*p) {
      case '<':
        out.operator+= <step> ("&lt;");
        break;
      case '>':
        out.operator+= <step> ("&gt;");
        break;
      case '&':
        out.operator+= <step> ("&amp;");
        break;
      case '\'':
        out.operator+= <step> ("&apos;");
        break;
      case '\"':
        out.operator+= <step> ("&quot;");
        break;
      default:
        continue;
      }
      *p = '\0';
      out.operator+= <step> (s);
      s = p + 1;
    }
    *p = '\0';
    out.operator+= <step> (s);
    s = p + 1;
  }

 public:
  N *next() const {
    return p_next;
  }

  string tag() const {
    return p_tag;
  }

  /**
   * @brief Set the tag of the node.
   *
   * @param doc  Reference to the document that ownes this node.
   * @param tag  New tag.
   */
  void set_tag (xml_allocator &doc, string const &tag) {
    p_tag = doc.txt.alloc (tag.len() + 1);
    memcpy (p_tag, tag.cstr(), tag.len());
  }

  string data() const {
    return p_data;
  }

  /**
   * @brief Set the data of the node.
   *
   * @param doc  Reference to the document that ownes this node.
   * @param data  New data.
   */
  void set_data (xml_allocator &doc, string const &data) {
    if (data) {
      p_data = doc.txt.alloc (data.len() + 1);
      memcpy (p_data, data.cstr(), data.len());
    } else {
      p_data = NULL;
    }
  }
};

class xml_attr : public xml_node<xml_attr, xml_elem> {
  friend class xml_elem;

 protected:
  template <int f>
  void parse (xml_allocator &allo, char *s, char **ep) {
    char *p = s;
    if (!skip (TAG_PRED, s, &p))
      throw xml_result (TAG, s);
    p_tag = s;
    // Check for =" syntax if wanted
    if (!(f & no_syntax)) {
      if (p[0] != '=' && p[1] != '\"')
        throw xml_result (SYNTAX, p);
    }
    *p         = '\0';
    char delim = p[1];
    s          = p + 2;
    parse_text<f> (allo, delim, s, &p);
    *p    = '\0';
    (*ep) = p + 1;
  }

  template <int s>
  xml_result print (string &out) {
    if (!p_tag)
      throw xml_result (NIL, "Incomplete attr");
    out.operator+= <s> (string::fmt ("%s=\"", p_tag));
    print_text<s> (out, string().view (p_data));
    out.operator+= <s> ("\"");
    if (p_next)
      out.operator+= <s> (" ");
    return OK;
  }
};

class xml_elem : public xml_node<xml_elem, xml_elem> {
 protected:
  xml_elem *p_tail;
  xml_elem *p_child;
  xml_elem *p_parent;
  xml_attr *p_attr;
  xml_attr *p_atail;

  void zero() {
    memset (this, 0, sizeof (xml_elem));
  }

  template <int f>
  void parse_end (int &leave, xml_elem *parent, char *s, char **ep) {
    char *p = s;
    if (!skip (TAG_PRED, s, &p))
      throw xml_result (TAG, s);
    if (!(f & no_tag_check)) {
      // Check if parent tag and parsed tag are identical
      if (!!strncmp (parent->p_tag, s, p - s)) {
        *p = '\0';
        throw xml_result (MISMATCH, string::fmt ("%s/%s", parent->p_tag, s));
      }
    }
    leave |= 1;
    (*ep) = ++p;
  }

  void parse_pi (char *s, char **ep) {
    // Dont really care
    skip_delim ('>', s, ep);
  }

  template <int f>
  void parse (xml_allocator &allo, xml_elem *parent, char *s, char **ep) {
    static int leave = 0;
    char      *p     = s;
    skip (SPACE_PRED, s, &p);
    if (*p != '<')
      throw xml_result (INCOMPLETE);
    p_parent = parent;
    s        = ++p;
    if (*p == '/')
      return parse_end<f> (leave, parent, p + 1, ep);
    if (*p == '?') {
      parse_pi (p + 1, &p);
      ++p;
      return parse<f> (allo, parent, p, ep);
    }
    if (!skip (TAG_PRED, s, &p))
      throw xml_result (TAG, s);
    char *pn = p;
    p_tag    = s;
    skip (SPACE_PRED, p, &p);
    while (*p != '>' && *p != '/' && *p) {
      xml_attr *attr = allo.nodes.alloc<xml::xml_attr>();
      attr->parse<f> (allo, p, &p);
      add_attr (attr);
      skip (SPACE_PRED, p, &p);
    }
    if (*p == '>') {
      *pn = '\0';
      s   = ++p;
      skip (SPACE_PRED, p, &p);
      parse_text<f> (allo, '<', p, &p);
      pn = p;
      while (1) {
        xml::xml_elem *celem = allo.nodes.alloc<xml::xml_elem>();
        celem->parse<f> (allo, this, p, &p);
        *pn = '\0';
        if (p_data && !leave)
          throw xml_result (TEXT_CHILD, p_tag);
        if (leave) {
          leave = 0;
          break;
        }
        add_child (celem);
      }
      (*ep) = p;
      return;
    } else if (*p == '/' || leave) {
      if (!parent)
        throw xml_result (ROOT);
      p += 1 + (*p == '/');
      leave = 0;
      *pn   = '\0';
      (*ep) = p;
      return;
    }
    throw xml_result (SYNTAX);
  }

 public:
  xml_elem() {
    zero();
  }

  /**
   * @brief Prints this node, and all subsequent nodes into `out`.
   *
   * @tparam s Number of bytes to allocate in out, whenever it must be extended.
   * @param level Level of the current node. Leave this untouched unless you
   * know what your are doing.
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int s = 32768>
  xml_result print (string &out, int level = 0) {
    try {
      if (!p_tag)
        throw xml_result (NIL, "Incomplete elem");
      if (level < 0)
        throw xml_result (LEVEL, string::fmt ("level=%i", level));
      if (level == 0)
        out.operator+= <s> ("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
      for (int i = 0; i < level; i++)
        out.operator+= <s> ("  ");
      out.operator+= <s> (string::fmt ("<%s", p_tag));
      if (p_attr) {
        out.operator+= <s> (" ");
        for (auto &a : attributes())
          a->print<s> (out);
      }
      if (!p_parent || p_data || p_child) {
        out.operator+= <s> (">");
        if (p_data) {
          print_text<s> (out, string().view (p_data));
        } else {
          out.operator+= <s> ("\n");
          for (auto &c : children())
            c->print (out, level + 1);
          for (int i = 0; i < level; i++)
            out.operator+= <s> ("  ");
        }
        out.operator+= <s> (string::fmt ("</%s>", p_tag));
        if (p_parent)
          out.operator+= <s> ("\n");
      } else {
        out.operator+= <s> ("/>");
        if (p_parent)
          out.operator+= <s> ("\n");
      }
    } catch (xml_result e) {
      return e;
    }
    return OK;
  }

  xml_elem *child() const {
    return p_child;
  }

  std::vector<xml_elem *> children() const {
    std::vector<xml_elem *> out;
    for (xml_elem *i = p_child; i; i = i->p_next)
      out.push_back (i);
    return out;
  }

  int num_attrs() const {
    int n = 0;
    for (xml_attr *i = p_attr; i; i = i->p_next)
      n++;
    return n;
  }

  std::vector<xml_attr *> attributes() const {
    std::vector<xml_attr *> out;
    for (xml_attr *i = p_attr; i; i = i->p_next)
      out.push_back (i);
    return out;
  }

  void add_attr (xml_attr *attr) {
    if (!attr)
      throw xml_result (ALLOC, "Null attr");
    // attr->p_parent = this;
    if (p_attr) {
      if (p_atail)
        p_atail->p_next = attr, p_atail = attr;
      else
        p_attr->p_next = attr, p_atail = attr;
      /*xml::xml_attr *i = p_attr;
      for (; i && i->p_next; i = i->p_next) {
        // Check for duplicate tag if wanted
      }
      i->p_next = attr;*/
    } else
      p_attr = attr;
  }

  void add_child (xml_elem *child) {
    if (!child)
      throw xml_result (ALLOC, "Null elem");
    child->p_parent = this;
    if (p_child) {
      xml::xml_elem *elem = p_child;
      if (elem->p_tail)
        elem->p_tail->p_next = child, elem->p_tail = child;
      else
        elem->p_next = child, elem->p_tail = child;
    } else {
      child->p_next = NULL;
      p_child       = child;
    }
  }
};

class xml_doc : public xml_elem, public xml_allocator {
  friend class xml_elem;

 protected:
  string source;

 public:
  /**
   * @brief Loads and parses an XML string into this document.
   * Accepts flags defined in the xml_flags enum.
   *
   * @tparam  f flags to be passed to the library.
   * @param content
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int f = none>
  xml_result load_string (string const &content) {
    this->zero();
    source = content;
    try {
      char *p = (char *)source.cstr();
      this->parse<f> (*this, NULL, p, &p);
    } catch (xml_result e) {
      nodes.free();
      return e;
    }
    return OK;
  }

  /**
   * @brief Loads and parses an XML file into this document.
   * Accepts flags defined in the xml_flags enum.
   *
   * @tparam  f flags to be passed to the library.
   * @param path
   * @param read  Pointer to a variable, that gets set to the number of bytes
   * read from the file.
   * @return  Non zero on failure, with an attached description.
   * Returns 0 on success.
   */
  template <int f = none>
  xml_result load_file (string const &path, long long *read = NULL) {
    std::ifstream fi (path.cstr());
    if (!fi)
      return FILE;
    fi >> source;
    if (read)
      (*read) += source.size();
    fi.close();
    if (!source)
      return FILE;
    return load_string<f> (source);
  }

  /**
   * @brief Creates a new attribute, owned by this document.
   *
   */
  xml_attr *new_attr (string const &tag, string const &data) {
    xml_attr *a = nodes.alloc<xml_attr>();
    a->set_tag (*this, tag);
    a->set_data (*this, data);
    return a;
  }

  /**
   * @brief Creates a new element, owned by this document.
   *
   */
  xml_elem *new_elem (string const &tag, string data = string()) {
    xml_elem *e = nodes.alloc<xml_elem>();
    e->set_tag (*this, tag);
    e->set_data (*this, data);
    return e;
  }
};
} // namespace xml
} // namespace scl