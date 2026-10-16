// The libFuzzer target for lenient parsing. Clang only (preset fuzz, which compiles with
// -fsanitize=fuzzer-no-link,address,undefined); the seed corpus runs through the same property as
// a normal test on every platform (corpus_test.cpp). A violation aborts, so libFuzzer reports it
// as a finding and saves the input.
//
// The input is parsed in place, not copied: libFuzzer hands over an exactly sized heap buffer, so
// ASan reports any read past its end.

#include "fuzz_property.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes viewed as char
    const std::string_view input(reinterpret_cast<const char*>(data), size);
    if (!nepdate_test::fuzz::parse_lenient_property(input)) {
        std::abort();
    }
    return 0;
}
