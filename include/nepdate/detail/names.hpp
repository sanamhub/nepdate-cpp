#ifndef NEPDATE_DETAIL_NAMES_HPP
#define NEPDATE_DETAIL_NAMES_HPP

// Digit writing shared by <nepdate/format.hpp> and <nepdate/fiscal_year.hpp> (ADR-0002 section 5).
// The name tables themselves are generated into names_tables.hpp.

#include <nepdate/date.hpp>
#include <nepdate/detail/names_tables.hpp> // IWYU pragma: export

#include <cstddef>
#include <cstdint>

namespace nepdate::detail {

// Returns the number of decimal digits of value, 1..10.
[[nodiscard]] constexpr int digit_count(std::uint32_t value) noexcept {
    int n = 1;
    while (value >= 10U) {
        value /= 10U;
        ++n;
    }
    return n;
}

// Bytes one digit takes: 1 for ASCII, 3 for Devanagari (U+0966..U+096F in UTF-8).
[[nodiscard]] constexpr std::size_t digit_bytes(lang l) noexcept {
    return l == lang::ne ? 3U : 1U;
}

// Writes value with at least min_width digits (leading zeros), ASCII or Devanagari. Returns the
// end of the output, or nullptr when [first, last) is too small; nothing is written then.
// Precondition: first <= last.
[[nodiscard]] constexpr char* write_number(char* first, const char* last, std::uint32_t value,
                                           int min_width, lang l) noexcept {
    const int count = digit_count(value);
    // min_width can be any int; widening keeps width * 3 from overflowing.
    const auto width = static_cast<std::size_t>(min_width > count ? min_width : count);
    const std::size_t bytes = width * digit_bytes(l);
    if (bytes / digit_bytes(l) != width || bytes > static_cast<std::size_t>(last - first)) {
        return nullptr;
    }
    char* const end = first + bytes;
    char* p = end;
    if (l == lang::ne) {
        // Devanagari digit k is U+0966 + k: the bytes E0 A5 (A6 + k).
        do { // NOLINT(cppcoreguidelines-avoid-do-while): body runs first             // which is the point of this loop
            const std::uint32_t k = value % 10U;
            value /= 10U;
            *--p = static_cast<char>(0xA6U + k);
            *--p = '\245';
            *--p = '\340';
        } while (value != 0U);
        while (p != first) {
            *--p = '\246';
            *--p = '\245';
            *--p = '\340';
        }
        return end;
    }
    while (value >= 100U) {
        const std::size_t i = static_cast<std::size_t>(value % 100U) * 2U;
        value /= 100U;
        *--p = kDigitPairs[i + 1U];
        *--p = kDigitPairs[i];
    }
    if (value >= 10U) {
        const std::size_t i = static_cast<std::size_t>(value) * 2U;
        *--p = kDigitPairs[i + 1U];
        *--p = kDigitPairs[i];
    } else {
        *--p = static_cast<char>('0' + value);
    }
    while (p != first) {
        *--p = '0';
    }
    return end;
}

} // namespace nepdate::detail

#endif // NEPDATE_DETAIL_NAMES_HPP
