#ifndef NEPDATE_FORMAT_HPP
#define NEPDATE_FORMAT_HPP

#include <nepdate/date.hpp>
#include <nepdate/detail/names.hpp>

#include <charconv>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace nepdate {

/// Writes value in decimal with at least min_width digits (leading zeros) into [first, last):
/// ASCII digits for lang::en, Devanagari digits (3 bytes each) for lang::ne. min_width counts
/// digits, not bytes. Returns {end of output, errc{}}, or {last, std::errc::value_too_large}
/// when the buffer is too small.
[[nodiscard]] constexpr std::to_chars_result
to_chars_number(char* first, char* last, unsigned value, int min_width, lang l) noexcept {
    char* const end =
        detail::write_number(first, last, static_cast<std::uint32_t>(value), min_width, l);
    if (end == nullptr) {
        return {last, std::errc::value_too_large};
    }
    return {end, std::errc{}};
}

/// Returns the month's name (FORMATTING section 1): "Shrawan" or the Nepali name. Returns ""
/// for a value outside 1..12.
[[nodiscard]] constexpr std::string_view month_name(month m, lang l) noexcept {
    const unsigned i = static_cast<unsigned>(m) - 1U;
    if (i >= 12U) {
        return {};
    }
    return l == lang::ne ? detail::kMonthNameNe[i] : detail::kMonthNameEn[i];
}

/// Returns the three-letter English name ("Shr"; D-04: "Asa" for Ashad, "Aso" for Ashoj), or the
/// full Nepali name for lang::ne, which has no short form. Returns "" for a value outside 1..12.
[[nodiscard]] constexpr std::string_view month_short_name(month m, lang l) noexcept {
    const unsigned i = static_cast<unsigned>(m) - 1U;
    if (i >= 12U) {
        return {};
    }
    return l == lang::ne ? detail::kMonthNameNe[i] : detail::kMonthShortEn[i];
}

/// Returns the weekday's name (FORMATTING section 1), or "" when !w.ok().
[[nodiscard]] constexpr std::string_view weekday_name(std::chrono::weekday w, lang l) noexcept {
    if (!w.ok()) {
        return {};
    }
    const unsigned i = w.c_encoding();
    return l == lang::ne ? detail::kWeekdayNameNe[i] : detail::kWeekdayNameEn[i];
}

/// Returns the short weekday name ("Tue" or the Nepali short form), or "" when !w.ok().
[[nodiscard]] constexpr std::string_view weekday_short_name(std::chrono::weekday w,
                                                            lang l) noexcept {
    if (!w.ok()) {
        return {};
    }
    const unsigned i = w.c_encoding();
    return l == lang::ne ? detail::kWeekdayShortNe[i] : detail::kWeekdayShortEn[i];
}

} // namespace nepdate

#endif // NEPDATE_FORMAT_HPP
