#ifndef NEPDATE_VERSION_HPP
#define NEPDATE_VERSION_HPP

// Licensed under the MIT License (see LICENSE).
// Kept equal to project() in CMakeLists.txt by tools/release_check.cmake (ADR-0005). The macros
// are public API (docs/API.md), so they stay macros rather than becoming enums or constexpr
// variables.
// NOLINTBEGIN(cppcoreguidelines-macro-to-enum, modernize-macro-to-enum)
#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 1
#define NEPDATE_VERSION_PATCH 0
// NOLINTEND(cppcoreguidelines-macro-to-enum, modernize-macro-to-enum)
// The semver pre-release part, without the leading "-"; empty for a stable version. project()
// holds numbers only, so it lives here and in NEPDATE_VERSION_STRING.
#define NEPDATE_VERSION_PRERELEASE "beta.1"
#define NEPDATE_VERSION_STRING "0.1.0-beta.1"

#endif // NEPDATE_VERSION_HPP
