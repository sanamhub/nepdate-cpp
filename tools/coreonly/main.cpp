// C4-04: the core-only program of gate 9 (ADR-0004 section 1). It uses only <nepdate/nepdate.hpp>
// and links nothing but nepdate::nepdate. The isolation gate builds it in Release and fails when
// its bytes contain patro data, so this program is the proof that the core carries none.
#include <nepdate/nepdate.hpp>

#include <array>
#include <cstddef>
#include <cstdio>

int main() {
    const nepdate::date d = nepdate::from_gregorian(2024, 7, 30).value();
    std::array<char, 16> buffer{};
    const std::to_chars_result r =
        nepdate::to_chars_iso(buffer.data(), buffer.data() + buffer.size(), d);
    buffer[static_cast<std::size_t>(r.ptr - buffer.data())] = '\n';
    std::fputs(buffer.data(), stdout);
    return 0;
}
