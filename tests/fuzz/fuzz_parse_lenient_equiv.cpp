// C7-02: the libFuzzer target for the parse_lenient equivalence property. Clang only (preset
// fuzz); it feeds every input to both the split parse_lenient and the C6 reference and aborts on
// any disagreement, so libFuzzer reports a finding and saves the input.

#include "../reference/parse_lenient_c6.hpp"

#include <nepdate/parse.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes viewed as char
    const std::string_view input(reinterpret_cast<const char*>(data), size);
    const nepdate::result<nepdate::date> split = nepdate::parse_lenient(input);
    const nepdate::result<nepdate::date> c6 = nepdate_test::reference::parse_lenient_c6(input);
    if (split != c6) {
        std::abort();
    }
    return 0;
}
