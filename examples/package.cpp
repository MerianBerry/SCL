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

int main(int argc, char **argv) {
  scl::init();
  const bool         write = false;

  scl::stream        one;
  scl::reduce_stream two;
  two.begin(scl::reduce_stream::Compress);
  two.write("Hi hello hello hello hello");
  two.end();
  two.seek(scl::StreamPos::start, 0);
  one.write(two);
  auto pos = one.tell();
  two.close(); // Reset stream
  two.begin(scl::reduce_stream::Compress);
  two.write("bye lol bye lol bye lol bye lol");
  two.end();
  one.write(two);
  auto pos2 = one.tell();
  two.close();

  char buf[1024];
  one.seek(scl::StreamPos::start, 0);
  auto r = one.read(buf, sizeof(buf));
  two.write_uncompressed(buf, r);
  two.seek(scl::StreamPos::start, 0);
  two.read(buf, 27);
  printf("%.*s\n", 27, buf);
#if 0
  scl::reduce_stream one;
  one.begin(scl::reduce_stream::Compress);
  one.write("Hi hello hello hello hello");
  one.end();
  auto               pos = one.tell();

  scl::reduce_stream two;
  two.begin(scl::reduce_stream::Compress);
  two.write("bye lol bye lol bye lol bye lol");
  two.end();
  two.seek(scl::StreamPos::start, 0);
  char buf[1024];
  auto r = two.read(buf, sizeof(buf));
  one.write_uncompressed(buf, r);
  auto pos2 = one.tell();

  one.seek(scl::StreamPos::start, 0);

  one.begin();
  r = one.read(buf, pos);
  one.end();
  one.seek(scl::StreamPos::start, pos);
  one.begin();
  r += one.read((char *)buf + r, pos2 - pos);
  one.end();
  printf("%.*s\n", (int)r, buf);
#endif

  scl::pack::Packager pack;
  pack.open("test.spk");

  pack.write();

#if 0
  std::vector<scl::path> files = {};
  // Add list of files
  auto headers  = scl::path::glob("src/*.hpp");
  auto csources = scl::path::glob("src/*.cpp");
  files.insert(files.end(), headers.begin(), headers.end());
  files.insert(files.end(), csources.begin(), csources.end());

  auto wts = pack.openFiles(files);
  scl::path::mkdir("build/extracted");

  using namespace scl::xml;

  double cst = scl::clock();
  for(size_t i = 0; i < wts.size(); i++) {
    auto &stream = wts[i]->stream();
    auto &file   = files[i];
    if(write) {
      // Open the pack entry stream to the actual file.
      // Filling the entry when the pack is written.
      if(!stream.open(file, scl::OpenMode::Rb)) {
        fprintf(stderr, "Failed to open %s\n", file.cstr());
        return 1;
      } else {
        long long size = stream.seek(scl::StreamPos::end, 0);
        stream.seek(scl::StreamPos::start, 0);

        fprintf(stderr, "Wrote %s (%0.lfkb)\n", file.cstr(), size / 1000.0);
      }
    } else {
      scl::stream out;
      auto        outfile = scl::path("build/extracted") / file;
      out.open(outfile, scl::OpenMode::Wtb);
      out.write(stream);
      long long size = out.tell();
      out.close();

      printf("Read %s (%0.lfkb)\n", file.cstr(), size / 1000.0);
    }
  }
  if(write)
    pack.write();
  double cet = scl::clock();
  printf("time: %0.2lfms\n", (cet - cst) * 1000.0);
#endif
  pack.close();
  scl::terminate();
  return 0;
}
