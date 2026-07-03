#include <scl_xml.hpp>
#include <scl_stream.hpp>
#include <scl_path.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

using namespace scl::xml;

#if 0
TEST_CASE("xml sample roundtrip", "[xml]") {
  scl::path path = scl::path::execdir() / "sample.xml";
  scl::stream ss(path, scl::OpenMode::READ);
  INFO(path);
  REQUIRE(ss.is_open());

  scl::string content;
  ss >> content;
  ss.close();
  REQUIRE(content.len() > 0);

  XmlDocument* doc = new XmlDocument();
  XmlResult r = doc->load_string(content);
  INFO(r.what());
  REQUIRE(r.code == OK);

  scl::string out;
  doc->print(out);
  delete doc;
  bool check = content == out;
  if(content != out) {
    ss.open(scl::path::execdir() / "fail_sample.xml", scl::OpenMode::WRITE);
    ss.write(out);
    ss.close();
    FAIL("in and out content is not identical. wrote to fail_sample.xml");
  }
}
#endif
