#include <sclc_string.h>
#include <sclc_swiss.h>
#include <sclc_time.h>
#include <string>
#include <unordered_map>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <math.h>

constexpr uint32_t n = 100000;

TEST_CASE("swiss_main", "[benchmark]") {
  srand(scl_clock());
  printf("scl_shmap_t\n");
  printf("generating %u keys...\n", n);
  const char** keys = (const char**)malloc(sizeof(char*) * n);
  for(int i = 0; i < n; i++) {
    keys[i] = strrand(10);
  }

  scl_shmap_t* map = scl_shnew();
  printf("populating hash map...\n");
  double cs = scl_clock();
  for(int i = 0; i < n; i++) {
    scl_shsets(map, keys[i], (void*)(intptr_t)i);
  }
  double ce = scl_clock();
  const double popt = ce - cs;

  printf("checking keys...\n");
  cs = scl_clock();
  for(int i = 0; i < n; i++) {
    if((intptr_t)scl_shgets(map, keys[i]) != (intptr_t)i) {
      printf("failed to find %s. test invalidated\n", keys[i]);
    }
  }
  ce = scl_clock();
  const double gett = ce - cs;
  printf("count: %u\ncapacity: %u\n", scl_shlength(map), scl_shcapacity(map));
  scl_shfree(map);
  printf("time:\n  populating: %0.2lfms\n  querying: %0.2lfms\n\n",
    popt * 1000,
    gett * 1000);
}

TEST_CASE("swiss_duplicate") {
  scl_shmap_t* map = scl_shnew();
  scl_shsets(map, "mykey", 0);
  scl_shsets(map, "mykey", (void*)1);
  CHECK(scl_shlength(map) == 1);
  CHECK(scl_shgets(map, "mykey") == (void*)1);
}

static int rand_int(int min, int max) {
  return (abs(rand()) % (max - min + 1)) + min;
}

TEST_CASE("swiss_remove") {
  const int N = 200;
  const char** keys = (const char**)malloc(sizeof(char*) * N);
  for(int i = 0; i < N; i++) {
    keys[i] = strrand(10);
  }

  scl_shmap_t* map = scl_shnew();
  for(int i = 0; i < N; i++) {
    scl_shsets(map, keys[i], (void*)(intptr_t)i);
  }

  for(int i = 0; i < 100; i++) {
    int x = rand_int(0, N - 1);
    if(!keys[x]) {
      i--;
      continue;
    }
    scl_shremoves(map, keys[x]);
    if(scl_shhass(map, keys[x])) {
      FAIL("map still has removed item");
    }
    keys[x] = NULL;
  }
}

#if 0
TEST_CASE("scl_htab") {
  printf("scl_htab (old)\n");
  printf("generating %u keys...\n", n);
  const char** keys = (const char**)malloc(sizeof(char*) * n);
  for(int i = 0; i < n; i++) {
    keys[i] = strrand(10);
  }

  sclo_htab* map = sclo_htabnew();
  printf("populating hash map...\n");
  double cs = scl_clock();
  for(int i = 0; i < n; i++) {
    sclo_htabset(map, keys[i], (void*)(intptr_t)(i + 1));
  }
  double ce = scl_clock();
  const double popt = ce - cs;

  printf("checking keys...\n");
  cs = scl_clock();
  for(int i = 0; i < n; i++) {
    if((intptr_t)sclo_htabget(map, keys[i]) != (intptr_t)i + 1) {
      printf("failed to find %s. test invalidated\n", keys[i]);
    }
  }
  ce = scl_clock();
  const double gett = ce - cs;
  // printf("count: %u\ncapacity: %u\n", scl_shlength(map),
  // scl_shcapacity(map)); scl_shfree(map);
  printf("time:\n  populating: %0.2lfms\n  querying: %0.2lfms\n\n",
    popt * 1000,
    gett * 1000);
}


TEST_CASE("unordered_main", "[benchmark]") {
  printf("std::unordered_map\n");
  printf("generating %u keys...\n", n);
  const char** keys = (const char**)malloc(sizeof(char*) * n);
  for(int i = 0; i < n; i++) {
    keys[i] = strrand(10);
  }

  std::unordered_map<std::string, int> map;
  printf("populating hash map...\n");
  double cs = scl_clock();
  for(int i = 0; i < n; i++) {
    map[std::string(keys[i])] = i;
  }
  double ce = scl_clock();
  const double popt = ce - cs;

  printf("checking keys...\n");
  cs = scl_clock();
  for(int i = 0; i < n; i++) {
    if(map.find(std::string(keys[i])) == map.end()) {
      printf("failed to find %s. test invalidated\n", keys[i]);
    }
  }
  ce = scl_clock();
  const double gett = ce - cs;
  printf("count: %zu\n", map.size());
  map.clear();
  printf("time:\n  populating: %0.2lfms\n  querying: %0.2lfms\n\n",
    popt * 1000,
    gett * 1000);
}
#endif
