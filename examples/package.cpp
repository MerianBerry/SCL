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
  // Open the pack family test.spk. Includes test_1.spk, etc.
  pack.open("test.spk");

  /* Example of reading files:
   * Prints the name of all files in the pack family.
   * Reads src/sclxml.hpp from the pack, and writes it to ./out.txt.
   * You can perform any operation on the stream you are given, this is just one
   * of them.
   * The openFile algorithm used here can be extended to multiple files
   * with openFiles, which returns a vector.
   */
#if 0
  // Print all known files in the pack.
  for(const auto& i : pack.index()) {
    fprintf(stderr, "File: %s\n", i->filepath().cstr());
  }

  // Request sclxml.hpp to be decompressed.
  pack
    .openFile("src/sclxml.hpp")
    // Get the decompressed data stream. This will wait for the data to be
    // decompressed.
    ->stream()
    // Open the decompressed data stream to out.txt, in binary mode (otherwise
    // it gets weird).
    ->open("out.txt", scl::OpenMode::WRITE, true);

/* Example of writing files:
 * Writes all files under src/ into a pack.
 */
#else
  // Get all files under src/
  auto files  = scl::path::glob("src/**");
  auto nfiles = files.size();
  // Open all those files
  auto indices = pack.openFiles(files);
  files.clear();
  // files.clear();
  for(auto i : indices) {
    // First, submit for writing, otherwise it will be ignored when writing.
    // If not opened here, it will automatically be opened and closed during
    // writing. It is reccomended to let scl open and close the file, as to not
    // allocate a bunch of memory before hand due to a trillion file streams.
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
  // Provide a callback per written file that does some cool logging
  pack.write([&](size_t id, scl::pack::PackIndex* idx) {
    // Add original file size
    ogtotal += idx->original();
    // Add compressed file size
    packed += idx->compressed();
    auto path = idx->filepath();
    if(path.len() > 80)
      path = path.substr(0, 80) + "...";
    // Ansi escape code stuffs
    printf("\x1b[5A");
    printf(
      "\x1b[2KTime elapsed: %0.2lfs\n"
      "\x1b[2KCompletion %0.2lf%%\n"
      "\x1b[2KWrote file (%zu) %s\n"
      "\x1b[2KTotal original size: %0.2lfmB\n"
      "\x1b[2KTotal compressed size: %0.2lfmB\n",
      (scl::clock() - cst), (id + 1) / (double)nfiles * 100.0, id + 1,
      path.cstr(), (ogtotal / 1024.0 / 1024.0), (packed / 1000.0 / 1000.0));
  });
#endif

  pack.close();
  return 0;
}
