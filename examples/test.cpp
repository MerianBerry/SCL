#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "sclpath.hpp"
#include "scldict.hpp"
#include "scljobs.hpp"

int main (int argc, char **argv) {
  scl::path p = "src/sclcore.cpp";
  auto      P = p.resolve();
  P           = P.resolve();
  auto r      = P.relative();
  auto R      = p.relative();
  return 0;
}
