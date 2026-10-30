#!/usr/bin/env bash
# Downloads a nepdate-cpp release from its GitHub release page and builds the consumer with CMake
# FetchContent, the way a user gets it, then runs it and requires "consumer OK".
#
#   scripts/verify-published.sh 0.1.0-beta.1
#   scripts/verify-published.sh latest
#   scripts/verify-published.sh latest --resolve
#   scripts/verify-published.sh latest --metadata
#
# latest is the newest non-draft release, pre-releases included while every release is one; the tag
# carries a v and the version here does not. --resolve prints the version alone and exits.
# --metadata reads the release metadata and sends HEAD requests to the three assets without
# downloading any of them, so the daily check adds no download to the release.
#
# The assets are fetched anonymously from the public release URL; only the metadata reads use
# GITHUB_TOKEN, when it is set, to stay under the API rate limit.
#
# verify-published.yml uses this script for the weekly check. A release run also checks the
# find_package path in the workflow, because that needs a second build from the installed archive.
set -euo pipefail

repository="sanamhub/nepdate-cpp"
api="https://api.github.com/repos/${repository}"
releases="https://github.com/${repository}/releases"
user_agent="nepdate-verify-published"

mode=install
version=""
while [ $# -gt 0 ]; do
  case "$1" in
    --resolve) mode=resolve; shift ;;
    --metadata) mode=metadata; shift ;;
    -*)
      echo "verify-published: unknown argument '$1'"
      exit 2
      ;;
    *)
      version="$1"
      shift
      ;;
  esac
done
version="${version:-latest}"

api_get() {
  local authorization=()
  if [ -n "${GITHUB_TOKEN:-}" ]; then
    authorization=(-H "Authorization: Bearer ${GITHUB_TOKEN}")
  fi
  curl -fsS --retry 3 -H "Accept: application/vnd.github+json" -H "User-Agent: ${user_agent}" \
    "${authorization[@]}" "$1"
}

latest_version() {
  api_get "${api}/releases?per_page=5" \
    | jq -r '[.[] | select(.draft == false)][0].tag_name' \
    | sed -e 's/^v//'
}

if [ "$version" = latest ]; then
  version="$(latest_version)"
  if [ "$mode" != resolve ]; then
    echo "verify-published: the newest release is ${version}"
  fi
fi
if [ "$mode" = resolve ]; then
  printf '%s\n' "$version"
  exit 0
fi

if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?$ ]]; then
  echo "verify-published: '$version' is not a version like 0.1.0 or 0.1.0-beta.1"
  exit 2
fi

archive="nepdate-cpp-${version}.tar.gz"
tag="v${version}"
checksum_files=(SHA256SUMS SHA512SUMS)

if [ "$mode" = metadata ]; then
  document="$(api_get "${api}/releases/tags/${tag}")"
  for name in "$archive" "${checksum_files[@]}"; do
    url="$(printf '%s' "$document" | jq -r --arg n "$name" '.assets[] | select(.name == $n) | .browser_download_url')"
    if [ -z "$url" ]; then
      echo "verify-published: release ${tag} does not list ${name}"
      exit 1
    fi
    code="$(curl -sS -o /dev/null -w '%{http_code}' -I -L --retry 3 "$url")"
    if [ "$code" != 200 ]; then
      echo "verify-published: ${url} answered ${code}"
      exit 1
    fi
    echo "verify-published: ${name} is listed and its URL answers 200"
  done
  exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/download"

echo "verify-published: downloading nepdate-cpp ${version} from ${releases}/tag/${tag}"
for name in "$archive" "${checksum_files[@]}"; do
  curl -fsSL --retry 3 -o "$work/download/$name" "${releases}/download/${tag}/$name"
done
if command -v sha256sum > /dev/null; then
  (cd "$work/download" && sha256sum -c SHA256SUMS && sha512sum -c SHA512SUMS)
else
  (cd "$work/download" && shasum -a 256 -c SHA256SUMS && shasum -a 512 -c SHA512SUMS)
fi

src="$work/source"
mkdir -p "$src"
tar -xzf "$work/download/$archive" -C "$src"
consumer="$work/consumer"
mkdir -p "$consumer"
cp "$src/nepdate-cpp-${version}/tests/consumer/consumer.cpp" "$consumer/"
sha256="$(awk -v n="$archive" '$2 == n { print $1 }' "$work/download/SHA256SUMS")"
if [ -z "$sha256" ]; then
  echo "verify-published: SHA256SUMS has no line for ${archive}"
  exit 1
fi
cat > "$consumer/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.25)
project(nepdate_fetch_consumer LANGUAGES CXX)
include(FetchContent)
FetchContent_Declare(nepdate
  URL "${releases}/download/${tag}/${archive}"
  URL_HASH SHA256=${sha256})
FetchContent_MakeAvailable(nepdate)
add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE nepdate::nepdate nepdate::patro)
target_compile_features(consumer PRIVATE cxx_std_20)
EOF
# CMake on Windows is a native program: it needs a C:/... path, not the /tmp/... of Git Bash.
cmake_path() {
  if command -v cygpath > /dev/null; then cygpath -m "$1"; else printf '%s' "$1"; fi
}
cmake -S "$(cmake_path "$consumer")" -B "$(cmake_path "$consumer/build")" \
  -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$(cmake_path "$consumer/bin")"
cmake --build "$(cmake_path "$consumer/build")" --config Release --parallel 2

exe="$(find "$consumer/bin" -type f \( -name consumer -o -name consumer.exe \) | head -n 1)"
if [ -z "$exe" ]; then
  echo "verify-published: the build produced no consumer binary"
  exit 1
fi
out="$("$exe" | tr -d '\r')"
echo "$out"
if [ "$out" != "consumer OK" ]; then
  echo "verify-published: expected 'consumer OK'"
  exit 1
fi
echo "verify-published: ok, nepdate-cpp ${version} from the release, built with FetchContent"
