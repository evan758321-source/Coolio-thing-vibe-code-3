#pragma once

// Compatibility shim for dependencies expecting fmt v10's <fmt/base.h>.
// QPM may currently restore fmt versions that provide <fmt/core.h> instead.
#include <fmt/core.h>
