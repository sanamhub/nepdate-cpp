#ifndef NEPDATE_PARSE_HPP
#define NEPDATE_PARSE_HPP

#include <nepdate/date.hpp>
#include <nepdate/detail/names_tables.hpp>

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

// The lenient first-pass separators (PARSING section 2): the strict ones minus '.', plus ','. The
// danda U+0964 is handled by lenient_sep_size.
inline constexpr std::string_view kLenientSeparators = "-/_\\ |,";

// Size in bytes of the lenient first-pass separator at s[i] (i < s.size()): 3 for the danda, 1 for
// an ASCII separator, 0 when the byte isn't one. Never reads past the end of s.
[[nodiscard]] constexpr std::size_t lenient_sep_size(std::string_view s, std::size_t i) noexcept {
    const auto c = static_cast<unsigned char>(s[i]);
    if (c == 0xE0U && s.size() - i >= 3 && static_cast<unsigned char>(s[i + 1]) == 0xA5U &&
        static_cast<unsigned char>(s[i + 2]) == 0xA4U) {
        return 3;
    }
    if (c < 0x80U && kLenientSeparators.find(static_cast<char>(c)) != std::string_view::npos) {
        return 1;
    }
    return 0;
}

// True when t is made only of ASCII digits, Devanagari digits and '.' (PARSING section 2). Such a
// token is re-split on dots, so "2080.05.15" gives three numbers while "B.S." stays one word.
[[nodiscard]] constexpr bool is_digits_and_dots(std::string_view t) noexcept {
    std::size_t i = 0;
    while (i < t.size()) {
        const auto c = static_cast<unsigned char>(t[i]);
        if (c == '.') {
            ++i;
        } else if (c >= '0' && c <= '9') {
            ++i;
        } else if (c == 0xE0U && t.size() - i >= 3 &&
                   static_cast<unsigned char>(t[i + 1]) == 0xA5U &&
                   static_cast<unsigned char>(t[i + 2]) >= 0xA6U &&
                   static_cast<unsigned char>(t[i + 2]) <= 0xAFU) {
            i += 3;
        } else {
            return false;
        }
    }
    return true;
}

// Reads a piece that is all ASCII or Devanagari digits (callers check with is_digits_and_dots
// first). Fills digits with the digit count and value with the integer, accumulated only while
// digits <= 4 so a very long token can't overflow.
constexpr void read_digits(std::string_view t, int& digits, int& value) noexcept {
    digits = 0;
    value = 0;
    std::size_t i = 0;
    while (i < t.size()) {
        const auto c = static_cast<unsigned char>(t[i]);
        int d = 0;
        std::size_t size = 1;
        if (c >= '0' && c <= '9') {
            d = static_cast<int>(c) - '0';
        } else {
            // The only other possibility: a Devanagari digit, E0 A5 (A6 + d).
            d = static_cast<int>(static_cast<unsigned char>(t[i + 2])) - 0xA6;
            size = 3;
        }
        i += size;
        ++digits;
        if (digits <= 4) {
            value = value * 10 + d;
        }
    }
}

// Compares a lenient token against a closed-list entry (PARSING sections 2 and 3). ASCII letters
// fold to lower case and the Kelvin sign U+212A folds to 'k' (the only code points Python
// str.lower() maps to ASCII); every other byte, including the dotless i U+0131, compares as-is, so
// Devanagari entries compare byte for byte and unknown scripts never match.
[[nodiscard]] constexpr bool folded_eq(std::string_view token, std::string_view entry) noexcept {
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < token.size()) {
        const auto c = static_cast<unsigned char>(token[i]);
        char folded = static_cast<char>(c);
        std::size_t size = 1;
        if (c >= 'A' && c <= 'Z') {
            folded = static_cast<char>(c - 'A' + 'a');
        } else if (c == 0xE2U && token.size() - i >= 3 &&
                   static_cast<unsigned char>(token[i + 1]) == 0x84U &&
                   static_cast<unsigned char>(token[i + 2]) == 0xAAU) {
            folded = 'k';
            size = 3;
        }
        if (j >= entry.size() || entry[j] != folded) {
            return false;
        }
        i += size;
        ++j;
    }
    return j == entry.size();
}

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

/// Parses human input (PARSING section 2). Strict parsing is tried first; otherwise the input is
/// normalised into at most three kept tokens on a fixed stack (ADR-0002 section 6): Devanagari
/// digits map to ASCII, ',' and the strict separators split tokens, a digits-and-dots token is
/// re-split on dots, and era markers plus the two dropped words (gate, miti) are dropped. A month
/// name with two numbers gives the date in any order, three numbers give Y M D or D M Y, and
/// anything else is unrecognized. Error order: number_too_long, ambiguous, then unrecognized; a
/// successful date then validates through make_date.
[[nodiscard]] constexpr result<date> parse_lenient(std::string_view s) noexcept {
    if (const result<date> r = parse(s); r.has_value()) {
        return r;
    }

    std::array<int, 3> nums{};
    std::array<int, 3> num_digits{};
    std::size_t num_count = 0;
    int month_value = 0;
    std::size_t month_count = 0;
    std::size_t unknown_count = 0;

    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t sep = detail::lenient_sep_size(s, i);
        if (sep != 0) {
            i += sep;
            continue;
        }
        const std::size_t start = i;
        while (i < s.size() && detail::lenient_sep_size(s, i) == 0) {
            ++i;
        }
        const std::string_view token = s.substr(start, i - start);
        if (detail::is_digits_and_dots(token)) {
            std::size_t p = 0;
            while (p <= token.size()) {
                const std::size_t dot = token.find('.', p);
                const std::size_t end = dot == std::string_view::npos ? token.size() : dot;
                if (end > p) {
                    int digits = 0;
                    int value = 0;
                    detail::read_digits(token.substr(p, end - p), digits, value);
                    if (digits > 4) {
                        return errc::number_too_long;
                    }
                    if (num_count < nums.size()) {
                        nums[num_count] = value;
                        num_digits[num_count] = digits;
                    }
                    ++num_count;
                }
                if (dot == std::string_view::npos) {
                    break;
                }
                p = end + 1;
            }
        } else {
            bool dropped = false;
            for (const std::string_view word : detail::kDroppedWords) {
                if (token == word) {
                    dropped = true;
                    break;
                }
            }
            if (!dropped) {
                for (const std::string_view era : detail::kEraMarkers) {
                    if (detail::folded_eq(token, era)) {
                        dropped = true;
                        break;
                    }
                }
            }
            if (dropped) {
                continue;
            }
            int m = 0;
            for (const detail::month_spelling& spelling : detail::kMonthSpellings) {
                if (detail::folded_eq(token, spelling.text)) {
                    m = static_cast<int>(spelling.month);
                    break;
                }
            }
            if (m != 0) {
                if (month_count == 0) {
                    month_value = m;
                }
                ++month_count;
            } else {
                ++unknown_count;
            }
        }
    }

    if (month_count == 1 && unknown_count == 0 && num_count == 2) {
        int year = 0;
        int day = 0;
        int day_digits = 0;
        if (num_digits[0] == 4 && num_digits[1] != 4) {
            year = nums[0];
            day = nums[1];
            day_digits = num_digits[1];
        } else if (num_digits[1] == 4 && num_digits[0] != 4) {
            year = nums[1];
            day = nums[0];
            day_digits = num_digits[0];
        } else {
            return errc::ambiguous;
        }
        if (day_digits > 2) {
            return errc::ambiguous;
        }
        return make_date(year, month_value, day);
    }
    if (month_count == 0 && unknown_count == 0 && num_count == 3) {
        if (num_digits[0] == 4) {
            return make_date(nums[0], nums[1], nums[2]);
        }
        if (num_digits[2] == 4) {
            return make_date(nums[2], nums[1], nums[0]);
        }
        return errc::ambiguous;
    }
    return errc::unrecognized;
}

/// Same as parse(std::string_view) on the same bytes, for u8"" literals.
[[nodiscard]] inline result<date> parse(std::u8string_view s) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): char8_t view as char (AGENTS.md)
    const auto* bytes = reinterpret_cast<const char*>(s.data());
    return parse(std::string_view(bytes, s.size()));
}

/// Same as parse_lenient(std::string_view) on the same bytes, for u8"" literals.
[[nodiscard]] inline result<date> parse_lenient(std::u8string_view s) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): char8_t view as char (AGENTS.md)
    const auto* bytes = reinterpret_cast<const char*>(s.data());
    return parse_lenient(std::string_view(bytes, s.size()));
}

} // namespace nepdate

#endif // NEPDATE_PARSE_HPP
