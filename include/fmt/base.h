#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace fmt {
using string_view = std::string_view;

template <typename Char>
using basic_string_view = std::basic_string_view<Char>;

template <typename Char>
class runtime_format_string {
 public:
  constexpr runtime_format_string(const Char*) {}
  constexpr runtime_format_string(std::basic_string_view<Char>) {}
};

struct format_args {};

inline std::string vformat(string_view, format_args) { return {}; }

template <typename... Args>
inline format_args make_format_args(Args&&...) { return {}; }

template <typename Char, typename... Args>
class basic_format_string {
 public:
  constexpr basic_format_string(const Char*) {}
  constexpr basic_format_string(std::basic_string_view<Char>) {}
};

template <typename... Args>
using format_string = basic_format_string<char, Args...>;

template <typename T, typename Char = char>
struct formatter {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const T&, FormatContext& ctx) const { return ctx.out(); }
};

template <typename... Args>
inline std::string format(const char*, Args&&...) {
  return {};
}
}  // namespace fmt
