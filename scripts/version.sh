#!/usr/bin/env bash
# Prints the version in include/nepdate/version.hpp, after checking the two CMake projects and the
# Conan recipes agree with it (hub ADR-0004 §3). A CMake project() holds numbers only, so the
# pre-release part lives in the header.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"

version="$(sed -n 's/^#define NEPDATE_VERSION_STRING "\(.*\)"/\1/p' "$root/include/nepdate/version.hpp" | head -n1)"
if [ -z "$version" ]; then
  echo "version: include/nepdate/version.hpp has no NEPDATE_VERSION_STRING" >&2
  exit 1
fi
core="${version%%-*}"

for project in "CMakeLists.txt:nepdate" "patro/CMakeLists.txt:nepdate-patro"; do
  file="${project%%:*}"
  name="${project##*:}"
  found="$(sed -n "s/^[[:space:]]*project($name VERSION \([0-9][0-9.]*\).*/\1/p" "$root/$file" | head -n1)"
  if [ "$found" != "$core" ]; then
    echo "version: $file project($name VERSION $found), want $core" >&2
    exit 1
  fi
done

for recipe in "$root"/conan/*/conanfile.py; do
  found="$(sed -n 's/^    version = "\(.*\)"/\1/p' "$recipe" | head -n1)"
  if [ "$found" != "$version" ]; then
    echo "version: ${recipe#"$root"/} has $found, want $version" >&2
    exit 1
  fi
done

printf '%s\n' "$version"
