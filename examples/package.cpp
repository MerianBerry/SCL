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


  scl::pack::Packager pack;
  pack.open("test.spk");
  auto files    = scl::path::glob("src/**/*.cpp");

  auto headers  = scl::path::glob("src/**/*.hpp");
  auto csources = scl::path::glob("src/**/*.c");
  auto cheaders = scl::path::glob("src/**/*.h");
  files.insert(files.end(), headers.begin(), headers.end());
  files.insert(files.end(), csources.begin(), csources.end());
  files.insert(files.end(), cheaders.begin(), cheaders.end());

  auto indices = pack.openFiles(files);
  for(auto i : indices) {
    // First, submit for writing, otherwise it will be ignored when writing.
    // Then, open the file in read mode, so that when the pack is written, it
    // will read the file into the pack.
    i->submit().open(scl::OpenMode::READ);
  }

  pack.write();

  scl::terminate();
  return 0;
}
