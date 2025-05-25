#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "sclpath.hpp"
#include "scljobs.hpp"

#pragma mini skip
#include <vector>

class IntWaitable : public scl::jobs::waitable {
  int m_res = 0;

 public:
  void set (int res) {
    lock();
    m_res = res;
    unlock();
  }

  int value() {
    wait();
    return m_res;
  }
};

class IntJob : public scl::jobs::job<IntWaitable> {
 public:
  IntWaitable *getWaitable() const override {
    return new IntWaitable;
  }

  void doJob (IntWaitable      *waitable,
    scl::jobs::jobworker const &worker) override {
    waitable->set (1);
  }
};

int main (int argc, char **argv) {
  scl::path val = "src/sclpath.cpp";
  val           = val.resolve();
  auto rel      = val.relative ("build");
  return 0;
}
