// The libFuzzer target for the parse_lenient equivalence property. Clang only (preset
// fuzz); it feeds every input to both the split parse_lenient and the reference and aborts on
// any disagreement, so libFuzzer reports a finding and saves the input.

#include "../reference/parse_lenient_reference.hpp"

#include <nepdate/parse.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes viewed as char
    const std::string_view input(reinterpret_cast<const char*>(data), size);
    const nepdate::result<nepdate::date> split = nepdate::parse_lenient(input);
    const nepdate::result<nepdate::date> reference =
        nepdate_test::reference::parse_lenient_reference(input);
    if (split != reference) {
        std::abort();
    }
    return 0;
}
