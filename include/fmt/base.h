#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace fmt {
using string_view = std::string_view;

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
