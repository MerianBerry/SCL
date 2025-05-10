#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"


#define MEGABYTE ((double)1024 * 1024)

struct unsigned_hash {
  static unsigned hash (unsigned const &key) {
    return key;
  }
};

class str_itr {};

#include <vector>

int main (int argc, char **argv) {
  char const *prop = "41f0d9";

  std::vector<int> v;
  {
    scl::string s;
    s.view (nullptr);
    auto files = scl::path::glob ("**/*.cpp");
    for (auto &i : files) {
      printf ("%s\n", i.cstr());
    }
  }
  return 0;
}
