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
  /*scl::pack::Packager pack;

  scl::stream s1, s2;
  s1.openMode ("archive.spk", "r");*/
  scl::path   file = "a10.xml";
  scl::stream fi;
  fi.open(file, false, true);
  long long us = fi.seek(scl::StreamPos::end, -1);
  fi.seek(scl::StreamPos::start, 0);

  double             ts = scl::clock();
  scl::reduce_writer rw;
  rw.open("compressed.bin", true, true);
  rw.begin();
  rw.write(fi);
  rw.end();
  double    te    = scl::clock();
  long long cs    = rw.tell();
  double    ratio = (double)cs / us;

  printf("File: %s\n", file.cstr());
  printf("Uncompressed size: %0.1lfkb\n", us / 1024.0);
  printf("Compressed size: %0.1lfkb\n", cs / 1024.0);
  printf("Compression Ratio: %0.0lf%%\n", ratio * 100);
  printf("Read + Compression + Write time: %0.2lfms\n", (te - ts) * 1000.0);

  // auto *file = pack.open ("fun.txt");
  //(*file)->write ("Hello\n");
  // delete file;

  // pack.write();

  return 0;
}
