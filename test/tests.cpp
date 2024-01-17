#include <anywho/concepts.hpp>
#include <anywho/direct_return.hpp>
#include <anywho/errors.hpp>
#include <anywho/with_context.hpp>
#include <catch2/catch_test_macros.hpp>
#include <expected>
#include <format>

namespace {
std::expected<int, std::string> myfuncUnexpected() { return std::unexpected("my_msg"); }

std::expected<int, std::string> myfuncValid() { return 3; }


std::expected<int, std::string> myfunc0()
{
  const int val = ANYWHO(myfuncUnexpected());
  return 3 * val;
}

std::expected<int, std::string> myfunc1()
{
  const int val = ANYWHO(myfuncValid());
  return 3 * val;
}

struct DummyError final
{
  [[nodiscard]] static std::string format() { return "lala"; }
  void consume_context(anywho::Context &&abc) { abc_ = std::move(abc); }
  [[nodiscard]] static std::string message() { return "dummy error happened"; }
  [[nodiscard]] static size_t id() { return std::hash<std::string>{}(message()); }

  anywho::Context abc_;
};

std::expected<int, DummyError> testError() { return std::unexpected(DummyError{}); }

}// namespace

TEST_CASE("error get returned with anywho", "[direct_return]")
{
  auto result = myfunc0();
  REQUIRE(!result.has_value());
  REQUIRE(result.error() == "my_msg");
}
TEST_CASE("value get returned with anywho", "[direct_return]")
{
  auto result = myfunc1();
  REQUIRE(result.has_value());
  REQUIRE(result.value() == 9);
}

TEST_CASE("fixed string", "[FixedString]")
{
  static constexpr int StringSize = 128;
  const anywho::FixedString<StringSize> test_string{ "abc" };
  REQUIRE(static_cast<std::string>(test_string) == "abc");
  const anywho::Context context2{ .message = "test", .line = 10, .file = "la.cpp" };
  REQUIRE(context2.format() == "la.cpp:10 -> test");
}

TEST_CASE("with_context", "[with_context]")
{
  static constexpr int line = 100;
  auto exp = anywho::with_context(testError(), { .message = "test", .line = line, .file = "test.cpp" });
  REQUIRE(!exp.has_value());
  REQUIRE(exp.error().format() == "lala");
}

TEST_CASE("test GenericError", "[errors]")
{
  anywho::GenericError err{};
  REQUIRE(err.format() == err.message());
  constexpr int line = 76;
  err.consume_context({ .message = "abc", .line = line, .file = "tests.cpp" });
  err.consume_context({ .message = "abc2", .line = line + 1, .file = "tests.cpp" });
  REQUIRE(err.format() == err.message() + "::tests.cpp:76 -> abc::tests.cpp:77 -> abc2");
}

TEST_CASE("test FixedSizeError", "[errors]")
{
  constexpr uint Size = 1024;
  anywho::FixedSizeError<Size> err{};
  REQUIRE(err.format() == err.message());
  constexpr int line = 76;
  err.consume_context({ .message = "abc", .line = line, .file = "tests.cpp" });
  err.consume_context({ .message = "abc2", .line = line + 1, .file = "tests.cpp" });
  REQUIRE(err.format() == err.message() + "::tests.cpp:76 -> abc::tests.cpp:77 -> abc2");
}

TEST_CASE("test FixedSizeError overflow", "[errors]")
{
  constexpr uint Size = 11;
  anywho::FixedSizeError<Size> err{};
  constexpr int line = 76;
  err.consume_context({ .message = "abc", .line = line, .file = "tests.cpp" });
  err.consume_context({ .message = "abc2", .line = line + 1, .file = "tests.cpp" });
  REQUIRE(err.format() == err.message() + "::tests.cp");
}