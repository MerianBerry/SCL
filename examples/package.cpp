#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "sclpath.hpp"
#include "sclpack.hpp"
#include "sclxml.hpp"
#include "sclreduce.hpp"

int main(int argc, char **argv) {
  scl::init();
  const bool          write = true;

  scl::pack::Packager pack;
  pack.open("test.spk");

  std::vector<scl::path> files = {};
#if 1
  auto headers  = scl::path::glob("**/*.hpp");
  auto csources = scl::path::glob("**/*.c");
  auto cheaders = scl::path::glob("**/*.h");
  files.insert(files.end(), headers.begin(), headers.end());
  files.insert(files.end(), csources.begin(), csources.end());
  files.insert(files.end(), cheaders.begin(), cheaders.end());
#endif

  auto   wts = pack.openFiles(files);

  double cst = scl::clock();
  for(size_t i = 0; i < wts.size(); i++) {
    auto &wt   = *wts[i];
    auto &file = files[i];
    if(write) {
      if(!wt->open(file, false, true)) {
        fprintf(stderr, "Failed to open %s\n", file.cstr());
        return 1;
      }
    } else {
      scl::string str;
      str.view((char *)wt->release());
      printf("Read %s\n", file.cstr());
    }
  }
  if(write)
    pack.write();
  double cet = scl::clock();
  printf("time: %0.2lfms\n", (cet - cst) * 1000.0);

  pack.close();
  scl::terminate();
  return 0;
}
