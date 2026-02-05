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
  two.seek(scl::StreamPos::start, 0);
  one.write(two);
  auto pos2 = one.tell();
  two.close();

  char buf[1024];
  one.seek(scl::StreamPos::start, 0);
  auto r = one.read(buf, sizeof(buf));
  two.write_uncompressed(buf, r);
  two.seek(scl::StreamPos::start, pos);
  two.begin();
  two.read(buf, 31);
  two.end();
  printf("%.*s\n", 31, buf);
  scl::terminate();
  return 0;
}
