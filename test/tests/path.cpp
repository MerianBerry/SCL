#include <scl_path.hpp>
#include <filesystem>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

namespace fs = std::filesystem;

SCENARIO("scl::path can resolve os paths") {
  WHEN("resolve() is called on an empty path") {
    scl::path path;
    THEN("the result will be null") {
      scl::path resolved = path.resolve();
      REQUIRE_FALSE(!!resolved);
    }
  }
  WHEN("resolve() is called on a valid path") {
    scl::path path = "./test";
    auto fspath = fs::absolute(path.cstr());
    THEN("the result will equal std::filesystem::absolute()") {
      scl::path resolved = path.resolve();
      REQUIRE(!!resolved);
      CHECK(resolved == fspath.generic_string());
    }
  }
}

SCENARIO("scl::path can get the parent components of a path") {
  WHEN("parentpath() is called on an empty path") {
    scl::path path;
    THEN("the result will be null") {
      scl::path parent = path.parentpath();
      CHECK_FALSE(!!parent);
    }
  }
  WHEN("parentpath() is called on a simple path string") {
    scl::path path = "foo/bar/fun.txt";
    auto fspath = fs::path(path.cstr()).parent_path();
    THEN("the result will equal std::filesystem::path::parent_path()") {
      auto parent = path.parentpath();
      REQUIRE(!!parent);
      CHECK(parent == fspath.generic_string());
    }
  }

  WHEN("parentpath() is called on a slight-malformed path string") {
    scl::path path = "foo//bar///fun.txt";
    auto fspath = fs::path(path.cstr()).parent_path();
    THEN("the result will equal std::filesystem::path::parent_path()") {
      auto parent = path.parentpath();
      REQUIRE(!!parent);
      CHECK(parent == fspath.generic_string());
    }
  }

  WHEN("parentpath() is called on a single component path string") {
    scl::path path = "foo";
    THEN("the result will be null") {
      auto parent = path.parentpath();
      REQUIRE_FALSE(!!parent);
    }
  }
}

SCENARIO("scl::path can get the extension component of a path") {
  WHEN("exension() is called on an empty path") {
    scl::path path;
    THEN("the result will be null") {
      auto ext = path.extension();
      REQUIRE_FALSE(!!ext);
    }
  }

  WHEN("extension() is called on a valid path string") {
    scl::path path = "foo/bar/fun.txt";
    THEN("the result will be correct") {
      auto ext = path.extension();
      REQUIRE(!!ext);
      CHECK(ext == ".txt");
    }
  }

  WHEN("extension() is called on a valid path string with multiple periods") {
    scl::path path = "foo/bar/fun.txt.bin";
    THEN("the result will be correct") {
      auto ext = path.extension();
      REQUIRE(!!ext);
      CHECK(ext == ".bin");
    }
  }
}

SCENARIO("scl::path can get the stem component of a path") {
  WHEN("stem() is called on an empty path") {
    scl::path path;
    THEN("the result will be null") {
      auto stem = path.stem();
      REQUIRE_FALSE(!!stem);
    }
  }

  WHEN("stem() is called on a valid path string") {
    scl::path path = "foo/bar/fun.txt";
    THEN("the result will be correct") {
      auto stem = path.stem();
      REQUIRE(!!stem);
      CHECK(stem == "fun");
    }
  }
}

SCENARIO("scl::path can get the individual components of a path") {
  WHEN("split() is called on an empty path") {
    scl::path path;
    THEN("the result will have no components") {
      std::vector<scl::path> components = path.split();
      REQUIRE(components.size() == 0);
    }
  }
  WHEN("split() is called on a valid path string") {
    scl::path path = "foo/bar/fun.txt";
    THEN("the result will have correct components") {
      std::vector<scl::path> components = path.split();
      REQUIRE(components.size() == 3);
      CHECK(components[0] == "foo");
      CHECK(components[1] == "bar");
      CHECK(components[2] == "fun.txt");
    }
  }
}

SCENARIO("scl::path can check if a path exists") {
  WHEN("exists() is called on an empty path") {
    scl::path path;
    THEN("the result will be false") {
      REQUIRE_FALSE(path.exists());
    }
  }
  WHEN("exists() is called on a valid file path") {
    scl::path path = "test/tests/path/hi.txt";
    THEN("the path will exist") {
      REQUIRE(path.exists());
    }
  }
}

SCENARIO("scl::path can check if a path is a file") {
  WHEN("isfile() is called on an empty path") {
    scl::path path;
    THEN("the result will be false") {
      REQUIRE_FALSE(path.isfile());
    }
  }
  WHEN("isfile() is called on a valid file path") {
    scl::path path = "test/tests/path/hi.txt";
    THEN("the file will exist") {
      REQUIRE(path.isfile());
    }
  }
}

SCENARIO("scl::path can check if a path is a directory") {
  WHEN("isdirectory() is called on an empty path") {
    scl::path path;
    THEN("the result will be false") {
      REQUIRE_FALSE(path.isdirectory());
    }
  }
  WHEN("isdirectory() is called on a valid directory path") {
    scl::path path = "test/tests/path";
    THEN("the directory will exist") {
      REQUIRE(path.isdirectory());
    }
  }
}
