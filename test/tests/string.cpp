#include <scl_string.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

SCENARIO("scl::string can view strings") {
  const char* teststr = "Hi im test string!";

  GIVEN("an empty string") {
    scl::string str;

    THEN("the size will be zero, and eval false") {
      CHECK(str.len() == 0);
      CHECK(str.size() == 0);
      CHECK_FALSE(str);
    }

    WHEN("view() is called") {
      str.view(teststr);

      THEN("the string is viewed") {
        CHECK(str == teststr);
        CHECK(str.len() == 18);
        CHECK(str.size() == 18);
        CHECK(str);
      }
    }

    WHEN("view(nullptr) is called") {
      str.view(nullptr);
      THEN("the size will be zero, and eval false") {
        CHECK(str.len() == 0);
        CHECK(str.size() == 0);
        CHECK_FALSE(str);
      }
    }
  }

  AND_GIVEN("a string constructed viewing a buffer") {
    scl::string str(teststr);

    THEN("the string is viewed") {
      CHECK(str == teststr);
      CHECK(str.len() == 18);
      CHECK(str.size() == 18);
      CHECK(str);
    }
  }
}

SCENARIO("string::reserve") {
  const char* teststr = "Hi im test string!";

  GIVEN("a filled string") {
    scl::string str(teststr);

    WHEN("the string is shrunk with reserve()") {
      str.reserve(5);

      THEN("the string will be smaller") {
        CHECK(str == "Hi im");
        CHECK(str.size() == 5);
      }
    }

    WHEN("the string is grown with reserve()") {
      str.reserve(20);

      THEN("the string will be bigger") {
        // manual length check
        CHECK(strlen(str.cstr()) == 18);
        CHECK(str.size() == 20);
        CHECK(str == teststr);
      }
    }

    WHEN("reserve(0) is called") {
      str.reserve(0);
      THEN("the string will be small and eval false") {
        CHECK(str.size() == 0);
        CHECK_FALSE(!!str);
      }
    }
  }

  GIVEN("an empty string") {
    scl::string str;
    WHEN("reserve is called with a positive size") {
      str.reserve(20);
      THEN("the string will have the correct size") {
        CHECK(str.size() == 20);
      }
    }
  }
}

SCENARIO("string::toInt") {
  const char* decimal = "12345";
  const char* decimalf = "1d2345";
  const char* hex = "0xdeadh3";

  GIVEN("a decimal string") {
    scl::string str(decimal);

    WHEN("toInt() is called") {
      int64_t i = str.toInt();
      THEN("the result will be correct") {
        CHECK(i == 12345);
      }
    }
  }

  GIVEN("a hexadecimal string") {
    scl::string str(hex);

    WHEN("toInt() is called") {
      int64_t i = str.toInt();

      THEN("the result will be correct") {
        CHECK(i == 0xdead);
      }
    }
  }

  GIVEN("a malformed decimal string") {
    scl::string str(decimalf);

    WHEN("toInt() is called") {
      int64_t i = str.toInt();

      THEN("the result will be correct") {
        CHECK(i == 1);
      }
    }
  }

  GIVEN("an empty string") {
    scl::string str;
    WHEN("toInt() is called") {
      int64_t i = str.toInt();
      THEN("the result will be 0") {
        CHECK(i == 0);
      }
    }
  }
}

SCENARIO("string::ffi") {
  GIVEN("a filled string") {
    scl::string str = "Hi im test string!";
    WHEN("ffi() is called with an existing pattern") {
      int32_t p = str.ffi("test");
      THEN("the result will be correct") {
        CHECK(p == 6);
      }
    }
    AND_WHEN("ffi() is called with a non-existing pattern") {
      int32_t p = str.ffi("blab");
      THEN("the result will be -1") {
        CHECK(p == -1);
      }
    }
  }
  GIVEN("an empty string") {
    scl::string str;
    WHEN("ffi() is called") {
      int32_t p = str.ffi("test");
      THEN("the result will be -1") {
        CHECK(p == -1);
      }
    }
  }
}

SCENARIO("string::fli") {
  GIVEN("a filled string") {
    scl::string str = "Hi string im test string!";
    WHEN("fli() is called with an existing pattern") {
      int32_t p = str.fli("string");
      THEN("the result will be correct") {
        CHECK(p == 18);
      }
    }
    AND_WHEN("ffi() is called with a non-existing pattern") {
      int32_t p = str.ffi("blab");
      THEN("the result will be -1") {
        CHECK(p == -1);
      }
    }
  }
  GIVEN("an empty string") {
    scl::string str;
    WHEN("fli() is called") {
      int32_t p = str.fli("string");
      THEN("the result will be -1") {
        CHECK(p == -1);
      }
    }
  }
}

SCENARIO("string::endswith") {
  GIVEN("a filled string") {
    scl::string str = "Hi im test string!";
    WHEN("endswith() is called with a correct pattern") {
      bool b = str.endswith("string!");
      THEN("the result will be true") {
        CHECK(b);
      }
    }
    AND_WHEN("endswith() is called with a false pattern") {
      bool b = str.endswith("blab");
      THEN("the result will be false") {
        CHECK_FALSE(b);
      }
    }
  }
  GIVEN("an empty string") {
    scl::string str;
    WHEN("endswith() is called") {
      bool b = str.endswith("string!");
      THEN("the result will be false") {
        CHECK_FALSE(b);
      }
    }
  }
}

SCENARIO("string::match") {
  GIVEN("a filled string") {
    scl::string str = "Hi im test string!";
    CHECK_FALSE(str.match("test"));
    CHECK_FALSE(str.match("terry"));
    CHECK_FALSE(str.match("*test"));
    CHECK_FALSE(str.match("*terry"));
    CHECK(str.match("*test*"));
    CHECK_FALSE(str.match("*terry*"));
    CHECK(str.match("Hi*"));
    CHECK_FALSE(str.match("string!*"));
  }

  GIVEN("an empty string") {
    scl::string str;
    CHECK_FALSE(str.match("Hi"));
  }
}

SCENARIO("string::substr") {
  GIVEN("a filled string") {
    scl::string str = "Hi im test string!";

    WHEN("substr() is called without max length") {
      auto sub = str.substr(0, 5);
      CHECK(sub == "Hi im");
    }
    WHEN("substr() is called with max length") {
      auto sub = str.substr(3);
      CHECK(sub == "im test string!");
    }
    WHEN("substr() is called with 0 length") {
      auto sub = str.substr(3, 0);
      CHECK(sub == "");
    }
    WHEN("substr() is called with out of bounds i") {
      auto sub = str.substr(INT_MAX);
      CHECK((sub == ""));
    }
  }
  GIVEN("an empty string") {
    scl::string str;
    WHEN("substr() is called") {
      auto sub = str.substr(0);
      CHECK(sub == "");
    }
  }
}

SCENARIO("string::replace(str,str)") {
  GIVEN("a filled string") {
    scl::string str = "Hi string im test string!";
    WHEN("replace() is called with a smaller replacement") {
      str.replace("string", "char");
      CHECK(str == "Hi char im test char!");
    }

    WHEN("replace() is called with a larger replacement") {
      str.replace("string", "johnney");
      CHECK(str == "Hi johnney im test johnney!");
    }
  }
}

SCENARIO("string::replace(str, int, int)") {
  GIVEN("a filled string") {
    scl::string str = "Hi im test string!";
    WHEN("replace() is called with i = 0 & in bounds j") {
      str.replace("Hey you", 0, 2);
      CHECK(str == "Hey you im test string!");
    }
    WHEN("replace() is called with in bounds i & out of bounds j") {
      str.replace("blab!", 6);
      CHECK(str == "Hi im blab!");
    }
    WHEN("replace() is called with out of bounds i") {
      str.replace("blab!", INT_MAX);
      THEN("nothing happens") {
        CHECK(str == "Hi im test string!");
      }
    }
  }
}
