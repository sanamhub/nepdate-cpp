#!/usr/bin/env bash
# Clones vcpkg at the builtin-baseline of vcpkg.json into $RUNNER_TEMP/vcpkg, bootstraps it and
# exports VCPKG_ROOT and VCPKG_BINARY_SOURCES to the following steps. The binary cache directory
# $RUNNER_TEMP/vcpkg-cache is saved and restored by actions/cache in each job (ADR-0004 §4).
#
# A fresh clone instead of the runner's preinstalled vcpkg: macOS runners don't ship one, and the
# preinstalled copies are at different commits on each image.
set -euo pipefail

baseline="$(jq -r '."builtin-baseline"' vcpkg.json)"
root="$RUNNER_TEMP/vcpkg"
cache="$RUNNER_TEMP/vcpkg-cache"

git clone --quiet https://github.com/microsoft/vcpkg.git "$root"
git -C "$root" checkout --quiet "$baseline"
if [ "${RUNNER_OS:-}" = "Windows" ]; then
    "$root/bootstrap-vcpkg.bat" -disableMetrics
else
    "$root/bootstrap-vcpkg.sh" -disableMetrics
fi
mkdir -p "$cache"

if [ -n "${GITHUB_ENV:-}" ]; then
    {
        echo "VCPKG_ROOT=$root"
        echo "VCPKG_BINARY_SOURCES=clear;files,$cache,readwrite"
    } >> "$GITHUB_ENV"
fi
