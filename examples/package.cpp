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
#if 1
  long long          us, cs, uns;
  double             cst, cet, ust, uet;
  double             ratio;

  scl::path          file       = "examples/horosont.glb";
  scl::path          compressed = "examples/compressed.bin";
  scl::reduce_stream rs;
  rs.open(compressed, true);
  {
    scl::stream fi;
    fi.open(file, false, true);

    rs.begin(scl::reduce_stream::Compress);
    cst = scl::clock();

    // Compress
    rs.write(fi);

    cet = scl::clock();

    rs.end();

    us = fi.tell();
    cs = rs.tell();
    fi.close();
  }
  rs.seek(scl::StreamPos::start, 0);
  ratio = (double)cs / us;

  {
    rs.begin();

    scl::stream un;
    ust = scl::clock();

    // Decompress
    un.reserve(us);
    un.write(rs);

    uet = scl::clock();
    rs.end();
    uns = un.tell();
    rs.close();
    un.close();
  }

  printf("File: %s\n", file.cstr());
  printf("Uncompressed size: %0.1lfkb\n", us / 1024.0);
  printf("Compressed size: %0.1lfkb\n", cs / 1024.0);
  printf("Compression Ratio: %0.0lf%%\n", ratio * 100);
  printf("Compression + Disk time: %0.2lfms\n", (cet - cst) * 1000.0);
  printf("Decompressed size: %0.1lfkb\n", uns / 1024.0);
  printf("Decompression + Disk read time: %0.2lfms\n", (uet - ust) * 1000.0);
#else
  const bool          write = true;

  scl::pack::Packager pack;
  pack.open("test.spk");

  std::vector<scl::path> files = {"examples/horosont.glb"};
#  if 0
  auto                   headers  = scl::path::glob("**/*.hpp");
  auto                   csources = scl::path::glob("**/*.c");
  auto                   cheaders = scl::path::glob("**/*.h");
  files.insert(files.end(), headers.begin(), headers.end());
  files.insert(files.end(), csources.begin(), csources.end());
  files.insert(files.end(), cheaders.begin(), cheaders.end());
#  endif

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

#endif
  scl::terminate();
  return 0;
}
