#ifndef NEPDATE_PARSE_HPP
#define NEPDATE_PARSE_HPP

#include <nepdate/date.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nepdate {

namespace detail {

// What one input position holds. Devanagari digits (U+0966..U+096F) and the danda U+0964 are the
// three bytes E0 A5 xx; a sequence cut off by the end of the input is `other`.
enum class char_kind : std::uint8_t { digit, separator, other };

struct scanned_char {
    char_kind kind;
    std::uint8_t digit; // 0..9 for a digit
    std::uint8_t size;  // bytes consumed, 1 or 3
};

// Reads the character at s[i], i < s.size(). separators lists the ASCII separators; the danda is
// always one. Never reads past the end of s.
[[nodiscard]] constexpr scanned_char scan_char(std::string_view s, std::size_t i,
                                               std::string_view separators) noexcept {
    const auto c = static_cast<unsigned char>(s[i]);
    if (c >= '0' && c <= '9') {
        return {char_kind::digit, static_cast<std::uint8_t>(c - '0'), 1};
    }
    if (c == 0xE0U && s.size() - i >= 3 && static_cast<unsigned char>(s[i + 1]) == 0xA5U) {
        const auto third = static_cast<unsigned char>(s[i + 2]);
        if (third >= 0xA6U && third <= 0xAFU) {
            return {char_kind::digit, static_cast<std::uint8_t>(third - 0xA6U), 3};
        }
        if (third == 0xA4U) {
            return {char_kind::separator, 0, 3};
        }
    }
    if (c < 0x80U && separators.find(static_cast<char>(c)) != std::string_view::npos) {
        return {char_kind::separator, 0, 1};
    }
    return {char_kind::other, 0, 1};
}

// The ASCII separators of strict parsing (PARSING section 1).
inline constexpr std::string_view kStrictSeparators = "-/._\\ |";

} // namespace detail

/// Parses "year month day" (PARSING section 1): groups of 1 to 4 ASCII or Devanagari digits (mixing
/// allowed, D-05), separated by any of - / . _ \ space | and the danda U+0964; leading, trailing
/// and repeated separators are ignored. Returns the date or invalid_character, wrong_group_count
/// (also for empty input), number_too_long, then out_of_range, invalid_month or invalid_day.
[[nodiscard]] constexpr result<date> parse(std::string_view s) noexcept {
    std::array<int, 3> groups{};
    std::size_t group = 0; // index of the group being read
    int digits = 0;        // digits in that group so far
    std::size_t i = 0;
    while (i < s.size()) {
        const detail::scanned_char c = detail::scan_char(s, i, detail::kStrictSeparators);
        i += c.size;
        if (c.kind == detail::char_kind::digit) {
            if (group == 3) {
                return errc::wrong_group_count;
            }
            if (digits == 4) {
                return errc::number_too_long;
            }
            groups[group] = groups[group] * 10 + c.digit;
            ++digits;
        } else if (c.kind == detail::char_kind::separator) {
            if (digits > 0) {
                ++group;
                digits = 0;
            }
        } else {
            return errc::invalid_character;
        }
    }
    if (digits > 0) {
        ++group;
    }
    if (group < 3) {
        return errc::wrong_group_count;
    }
    return make_date(groups[0], groups[1], groups[2]);
}

/// Same as parse(std::string_view) on the same bytes, for u8"" literals.
[[nodiscard]] inline result<date> parse(std::u8string_view s) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): char8_t view as char (AGENTS.md)
    const auto* bytes = reinterpret_cast<const char*>(s.data());
    return parse(std::string_view(bytes, s.size()));
}

} // namespace nepdate

#endif // NEPDATE_PARSE_HPP
