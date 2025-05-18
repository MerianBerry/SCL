#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include "sclpack.hpp"

int main (int argc, char **argv) {
  scl::pack::Collection col;
  auto                 &F = col.open ("map.xml");
  F.write ("Hello", 6);
  return 0;
}
