#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include "sclpack.hpp"

template <class T>
class yah {};

class no : yah<no> {};

int main (int argc, char **argv) {
  scl::pack::Packager pack;
  auto               *wt = pack.open ("fun.xml");
  (*wt)->write ("Hello\n");

  char buf[256];
  memset (buf, 0, sizeof (buf));

  (*wt)->seek (scl::StreamPos::start, 0);
  (*wt)->read (buf, -1);

  delete wt;

  printf ("%s", buf);

  return 0;
}
