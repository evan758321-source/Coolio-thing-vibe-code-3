#pragma once

// Minimal fallback shim used when fmt headers are unavailable in restored externs.
// Provides enough surface for logging headers to compile in CI.
#include <string>
#include <utility>

namespace fmt {
template <typename... Args>
class format_string {
 public:
  constexpr explicit format_string(const char*) {}
};

template <typename... Args>
inline std::string format(const char*, Args&&...) {
  return {};
}
}  // namespace fmt
