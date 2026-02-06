#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "sclpath.hpp"
#include "sclpack.hpp"
#include "sclxml.hpp"
#include "sclreduce.hpp"

#include <iostream>

int main(int argc, char** argv) {
  scl::init();


#if 1
  scl::pack::Packager pack;
  pack.open("test.spk");
#endif

  scl::string l     = "..";
  scl::string r     = l;
  auto        files = scl::path::glob("../vesuvius/**");

  printf("%zu files\n", files.size());

#if 1
  auto indices = pack.openFiles(files);
#endif
  files.clear();

#if 1
  // files.clear();
  for(auto i : indices) {
    // First, submit for writing, otherwise it will be ignored when writing.
    // Then, open the file in read mode, so that when the pack is written, it
    // will read the file into the pack.
    i->submit().open(scl::OpenMode::READ, true);
  }

  pack.write();

  size_t ogtotal = 0;
  size_t packed  = 0;
  for(const auto& i : pack.index()) {
    ogtotal += i->original();
    packed += i->compressed();
  }

  fprintf(stderr, "Original size: %0.2lfmB\nCompressed size: %0.2lfmB\n",
    (ogtotal / 1024.0 / 1024.0), (packed / 1000.0 / 1000.0));
  pack.close();
#endif

  scl::terminate();
  return 0;
}
