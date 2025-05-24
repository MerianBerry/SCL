#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "sclcore.hpp"
#include "scldict.hpp"
#include "sclpath.hpp"
#include "scljobs.hpp"

#include <windows.h>

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

  int doJob (IntWaitable       *waitable,
    scl::jobs::jobworker const &worker) override {
    waitable->set (1);
    return 0;
  }
};

int main (int argc, char **argv) {
  scl::jobs::jobserver serv;
  serv.start();
  for (int i = 0; i < 32; i++) {
    serv.submitJob ([=] (scl::jobs::jobworker const &worker) {
      scl::waitms (50);
      system (scl::string::fmt ("echo Hello from job %i", i).cstr());
      // printf ("Hello from worker %i, doing job %i\n", worker.id(), i);
    });
  }
  serv.waitidle();
  serv.stop();
  return 0;
}
