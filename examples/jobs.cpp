
#include "sclcore.hpp"
#include "scljobs.hpp"
#include <vector>
#include <iostream>

int main(int argc, char** argv) {
  scl::init();

  scl::jobs::JobServer serv;
  serv.start();

  constexpr int              n = 20;
  scl::jobs::promise<double> promises[n];
  for(int i = 0; i < n; i++) {
    const auto lambda = [](int id) {
      fprintf(stderr, "waiting %i\n", id);
      scl::waitms(2000);
      return (double)id;
    };
    promises[i] = serv.async(lambda, i);
  }
  for(int i = 0; i < n; i++) {
    double x = promises[i]->yield();
    printf("%lf\n", x);
  }
  std::cout << "done\n";
}
