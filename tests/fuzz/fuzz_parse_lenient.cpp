// C3-07: the libFuzzer target for lenient parsing. Clang only (preset fuzz, which compiles with
// -fsanitize=fuzzer-no-link,address,undefined); the seed corpus runs through the same property as
// a normal test on every platform (corpus_test.cpp). A violation aborts, so libFuzzer reports it
// as a finding and saves the input.

#include "fuzz_property.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {

// libFuzzer's default -max_len. Longer inputs are still covered, in pieces.
constexpr std::size_t kMaxInput = 4096;

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::size_t off = 0;
    do {
        std::array<char, kMaxInput> buf{};
        const std::size_t n = std::min(kMaxInput, size - off);
        if (n != 0) {
            std::memcpy(buf.data(), data + off, n);
        }
        if (!nepdate_test::fuzz::parse_lenient_property(std::string_view(buf.data(), n))) {
            std::abort();
        }
        off += n;
    } while (off < size);
    return 0;
}
