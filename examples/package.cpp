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
  auto        files  = scl::path::glob("src/**");
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
    i->submit();
  }


  // Some CLI printing
  printf(
    "Time elapsed: 0.00s\n"
    "Completion 0.00%%\n"
    "Wrote file 1\n"
    "Total original size: 0mB\n"
    "Total compressed size: 0mB\n");
  size_t ogtotal = 0;
  size_t packed  = 0;
  // Write all submitted files
  double cst = scl::clock();
  pack.write([&](size_t id, scl::pack::PackIndex* idx) {
#  if 1
    ogtotal += idx->original();
    packed += idx->compressed();
    auto path = idx->filepath();
    if(path.len() > 80) {
      path = path.substr(0, 80) + "...";
    }
    printf("\x1b[5A");
    printf(
      "\x1b[2KTime elapsed: %0.2lfs\n"
      "\x1b[2KCompletion %0.2lf%%\n"
      "\x1b[2KWrote file (%zu) %s\n"
      "\x1b[2KTotal original size: %0.2lfmB\n"
      "\x1b[2KTotal compressed size: %0.2lfmB\n",
      (scl::clock() - cst), (id + 1) / (double)nfiles * 100.0, id + 1,
      path.cstr(), (ogtotal / 1024.0 / 1024.0), (packed / 1000.0 / 1000.0));
#  endif
  });
  pack.close();
#endif

  scl::terminate();
  return 0;
}
