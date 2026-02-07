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

  scl::string l      = "..";
  scl::string r      = l;
  auto        files  = scl::path::glob("../Cyclone-Engine/**");

  size_t      nfiles = files.size();

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


  // Some CLI printing
  printf("\x1b[s");
  printf(
    "Completion 0.00%%\nWrote file 1\nTotal original size: 0mB\nTotal "
    "compressed size: "
    "0mB\n");
  size_t ogtotal = 0;
  size_t packed  = 0;
  // Write all submitted files
  pack.write([&](size_t id, scl::pack::PackIndex* idx) {
    ogtotal += idx->original();
    packed += idx->compressed();
    printf("\x1b[4A");
    printf(
      "\x1b[2KCompletion %0.2lf%%\n\x1b[2KWrote file (%zu) %s\n\x1b[2KTotal "
      "original size: "
      "%0.2lfmB\n\x1b[2KTotal "
      "compressed size: "
      "%0.2lfmB\n",
      (id + 1) / (double)nfiles * 100.0, id + 1, idx->filepath().cstr(),
      (ogtotal / 1024.0 / 1024.0), (packed / 1000.0 / 1000.0));
  });
  pack.close();
#endif

  scl::terminate();
  return 0;
}
