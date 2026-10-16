#ifndef NEPDATE_FORMAT_HPP
#define NEPDATE_FORMAT_HPP

#include <nepdate/date.hpp>
#include <nepdate/detail/names.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <version>

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

/// Field order of a short format (FORMATTING section 3, C# DateFormats in the same order).
enum class date_order : std::uint8_t { ymd, ydm, myd, mdy, dym, dmy };

/// Separator of a short format: slash, backslash, dot, underscore, dash or space.
enum class separator : std::uint8_t { slash, backslash, dot, underscore, dash, space };

/// Options of to_chars_short. pad gives a 4-digit year and 2-digit month and day.
struct short_format {
    date_order order = date_order::ymd;
    separator sep = separator::slash;
    bool pad = true;
};

/// Options of to_chars_long: "[<weekday>, ]<month> <day>[, <year>]". The defaults are C#'s.
struct long_format {
    bool weekday = false;
    bool year = true;
    bool pad = true; ///< 2-digit day
};

/// Bytes any short_format output fits in, in either language (at most 26 are used).
inline constexpr std::size_t max_short_size = 30;
/// Bytes any long_format output fits in, in either language (at most 68 are used).
inline constexpr std::size_t max_long_size = 80;

namespace detail {

// Output into [p, last). The first write that doesn't fit sets ok to false and p to last, so
// every later write fails too and nothing lands past last.
struct span_sink {
    char* p = nullptr;
    char* last = nullptr;
    bool ok = true;

    constexpr void fail() noexcept {
        p = last;
        ok = false;
    }
    constexpr void put(char c) noexcept {
        if (p == last) {
            fail();
            return;
        }
        *p++ = c;
    }
    constexpr void write(std::string_view s) noexcept {
        if (s.size() > static_cast<std::size_t>(last - p)) {
            fail();
            return;
        }
        p = std::copy_n(s.data(), s.size(), p);
    }
    constexpr void number(std::uint32_t value, int min_width, lang l) noexcept {
        char* const end = write_number(p, last, value, min_width, l);
        if (end == nullptr) {
            fail();
            return;
        }
        p = end;
    }
    [[nodiscard]] constexpr std::to_chars_result result() const noexcept {
        if (!ok) {
            return {last, std::errc::value_too_large};
        }
        return {p, std::errc{}};
    }
};

// Counts the bytes a format writes, so a std::string can be sized before writing.
struct count_sink {
    std::size_t n = 0;

    constexpr void put(char /*c*/) noexcept { ++n; }
    constexpr void write(std::string_view s) noexcept { n += s.size(); }
    constexpr void number(std::uint32_t value, int min_width, lang l) noexcept {
        const int count = digit_count(value);
        n += static_cast<std::size_t>(min_width > count ? min_width : count) * digit_bytes(l);
    }
};

[[nodiscard]] constexpr char separator_char(separator s) noexcept {
    switch (s) {
    case separator::backslash:
        return '\\';
    case separator::dot:
        return '.';
    case separator::underscore:
        return '_';
    case separator::dash:
        return '-';
    case separator::space:
        return ' ';
    case separator::slash:
        break;
    }
    return '/';
}

// FORMATTING section 3. A value outside the enumerators counts as ymd or slash.
template <class Sink>
constexpr void write_short(Sink& out, date d, short_format f, lang l) noexcept {
    struct field {
        std::uint32_t value;
        int width;
    };
    const field y{static_cast<std::uint32_t>(d.year()), f.pad ? 4 : 1};
    const field m{static_cast<std::uint32_t>(d.month()), f.pad ? 2 : 1};
    const field dd{static_cast<std::uint32_t>(d.day()), f.pad ? 2 : 1};
    field a = y;
    field b = m;
    field c = dd;
    switch (f.order) {
    case date_order::ydm:
        b = dd;
        c = m;
        break;
    case date_order::myd:
        a = m;
        b = y;
        break;
    case date_order::mdy:
        a = m;
        b = dd;
        c = y;
        break;
    case date_order::dym:
        a = dd;
        b = y;
        c = m;
        break;
    case date_order::dmy:
        a = dd;
        c = y;
        break;
    case date_order::ymd:
        break;
    }
    const char sep = separator_char(f.sep);
    out.number(a.value, a.width, l);
    out.put(sep);
    out.number(b.value, b.width, l);
    out.put(sep);
    out.number(c.value, c.width, l);
}

// FORMATTING section 4.
template <class Sink> constexpr void write_long(Sink& out, date d, long_format f, lang l) noexcept {
    if (f.weekday) {
        out.write(weekday_name(d.weekday(), l));
        out.write(", ");
    }
    out.write(month_name(d.month(), l));
    out.put(' ');
    out.number(static_cast<std::uint32_t>(d.day()), f.pad ? 2 : 1, l);
    if (f.year) {
        out.write(", ");
        out.number(static_cast<std::uint32_t>(d.year()), 4, l);
    }
}

// FORMATTING section 5: the standard patterns when the whole pattern is one of them, otherwise a
// left-to-right scan. Token digits follow l; literal bytes are copied unchanged (D-15).
// NOLINTBEGIN(readability-function-cognitive-complexity): one branch per FORMATTING section 5 token, in
// the spec's order, so the code reads against the table; 451 pattern vectors cover it.
template <class Sink>
constexpr void write_pattern(Sink& out, date d, std::string_view p, lang l) noexcept {
    if (p.empty() || p == "G" || p == "g" || p == "d") {
        write_short(out, d, short_format{}, l);
        return;
    }
    if (p == "D") {
        write_long(out, d, long_format{}, l);
        return;
    }
    if (p == "s") {
        write_short(out, d, short_format{date_order::ymd, separator::dash, true}, l);
        return;
    }
    const auto year = static_cast<std::uint32_t>(d.year());
    const auto m = static_cast<std::uint32_t>(d.month());
    const auto day = static_cast<std::uint32_t>(d.day());
    const std::size_t n = p.size();
    std::size_t i = 0;
    while (i < n) {
        const char c = p[i];
        if (c == '\\') {
            // A '\' at the very end writes nothing.
            if (i + 1 < n) {
                out.put(p[i + 1]);
            }
            i += 2;
        } else if (c == '\'') {
            // An unterminated quote runs to the end of the pattern.
            std::size_t close = p.find('\'', i + 1);
            if (close == std::string_view::npos) {
                close = n;
            }
            out.write(p.substr(i + 1, close - i - 1));
            i = close + 1;
        } else if (c == 'y' || c == 'M' || c == 'd') {
            std::size_t j = i + 1;
            while (j < n && p[j] == c) {
                ++j;
            }
            const std::size_t run = j - i;
            i = j;
            if (c == 'y') {
                if (run >= 4) {
                    out.number(year, 4, l);
                } else {
                    out.number(year % 100U, 2, l);
                }
            } else if (c == 'M') {
                if (run >= 4) {
                    out.write(month_name(d.month(), l));
                } else if (run == 3) {
                    out.write(month_short_name(d.month(), l));
                } else {
                    out.number(m, static_cast<int>(run), l);
                }
            } else {
                out.number(day, run == 2 ? 2 : 1, l);
            }
        } else {
            // Copy the literal run up to the next special character in one write.
            std::size_t j = i + 1;
            while (j < n && p[j] != '\\' && p[j] != '\'' && p[j] != 'y' && p[j] != 'M' &&
                   p[j] != 'd') {
                ++j;
            }
            out.write(p.substr(i, j - i));
            i = j;
        }
    }
}
// NOLINTEND(readability-function-cognitive-complexity)

} // namespace detail

// Every to_chars_* below writes into [first, last) without allocating and returns
// {end of output, errc{}}, or {last, std::errc::value_too_large} when the buffer is too small (the
// buffer contents are then unspecified, but nothing is written past last). Nothing is
// null-terminated.

/// Writes the short format of d (FORMATTING section 3). An order or separator outside the
/// enumerators counts as ymd or slash.
[[nodiscard]] constexpr std::to_chars_result to_chars_short(char* first, char* last, date d,
                                                            short_format f, lang l) noexcept {
    detail::span_sink out{first, last};
    detail::write_short(out, d, f, l);
    return out.result();
}

/// Writes "2081/04/15" (FORMATTING section 2; C# ToString()).
[[nodiscard]] constexpr std::to_chars_result to_chars(char* first, char* last, date d) noexcept {
    return to_chars_short(first, last, d, short_format{}, lang::en);
}

/// Writes "2081-04-15", the interchange form (D-11).
[[nodiscard]] constexpr std::to_chars_result to_chars_iso(char* first, char* last,
                                                          date d) noexcept {
    return to_chars_short(first, last, d, short_format{date_order::ymd, separator::dash, true},
                          lang::en);
}

/// Writes the long format of d (FORMATTING section 4), for example "Tuesday, Shrawan 15, 2081".
[[nodiscard]] constexpr std::to_chars_result to_chars_long(char* first, char* last, date d,
                                                           long_format f, lang l) noexcept {
    detail::span_sink out{first, last};
    detail::write_long(out, d, f, l);
    return out.result();
}

/// Writes d with a pattern (FORMATTING section 5): "", "G", "g", "d" give "2081/04/15", "D" the
/// long format, "s" "2081-04-15"; any other pattern is scanned for the tokens yyyy, yy, MMMM, MMM,
/// MM, M, dd, d, '...' and a backslash that escapes the next character. Token digits and names
/// follow l; other characters are copied.
[[nodiscard]] constexpr std::to_chars_result
to_chars_pattern(char* first, char* last, date d, std::string_view pattern, lang l) noexcept {
    detail::span_sink out{first, last};
    detail::write_pattern(out, d, pattern, l);
    return out.result();
}

/// Returns "2081/04/15". Fits the small-string buffer of every standard library, so it doesn't
/// allocate.
[[nodiscard]] inline std::string to_string(date d) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars(buf.data(), buf.data() + buf.size(), d);
    return {buf.data(), r.ptr};
}

/// Returns "2081-04-15". Allocates only the returned string.
[[nodiscard]] inline std::string to_iso_string(date d) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_iso(buf.data(), buf.data() + buf.size(), d);
    return {buf.data(), r.ptr};
}

/// Returns what to_chars_short writes. Allocates only the returned string.
[[nodiscard]] inline std::string format_short(date d, short_format f, lang l) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_short(buf.data(), buf.data() + buf.size(), d, f, l);
    return {buf.data(), r.ptr};
}

/// Returns what to_chars_long writes. Allocates only the returned string.
[[nodiscard]] inline std::string format_long(date d, long_format f, lang l) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_long(buf.data(), buf.data() + buf.size(), d, f, l);
    return {buf.data(), r.ptr};
}

/// Returns what to_chars_pattern writes, however long. Allocates only the returned string.
[[nodiscard]] inline std::string format_pattern(date d, std::string_view pattern, lang l) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r =
        to_chars_pattern(buf.data(), buf.data() + buf.size(), d, pattern, l);
    if (r.ec == std::errc{}) {
        return {buf.data(), r.ptr};
    }
    // Longer than the stack buffer: measure, then write straight into the string.
    detail::count_sink count;
    detail::write_pattern(count, d, pattern, l);
    std::string out(count.n, '\0');
    detail::span_sink sink{out.data(), out.data() + out.size()};
    detail::write_pattern(sink, d, pattern, l);
    return out;
}

} // namespace nepdate

// The one feature-test #if of the library (docs/API.md): std::formatter exists only where the
// standard library has <format>.
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>

namespace nepdate::detail {

// Output to a std::format iterator through a 256-byte stack buffer, flushed when full, so a
// long result is written in pieces and never truncated.
template <class OutputIt> struct iterator_sink {
    OutputIt out;
    std::array<char, 256> buf{};
    std::size_t n = 0;

    void flush() {
        out = std::copy_n(buf.data(), n, out);
        n = 0;
    }
    void put(char c) {
        if (n == buf.size()) {
            flush();
        }
        buf[n++] = c;
    }
    void write(std::string_view s) {
        while (!s.empty()) {
            if (n == buf.size()) {
                flush();
            }
            const std::size_t k = std::min(buf.size() - n, s.size());
            std::copy_n(s.data(), k, buf.data() + n);
            n += k;
            s.remove_prefix(k);
        }
    }
    void number(std::uint32_t value, int min_width, lang l) {
        // Pattern tokens ask for at most 4 digits of a value below 2^32: 30 bytes at most.
        std::array<char, 32> digits{};
        char* const end =
            write_number(digits.data(), digits.data() + digits.size(), value, min_width, l);
        if (end != nullptr) {
            write(std::string_view(digits.data(), static_cast<std::size_t>(end - digits.data())));
        }
    }
};

} // namespace nepdate::detail

/// {} prints "2081/04/15"; {:<pattern>} formats with to_chars_pattern and lang::en (FORMATTING
/// section 5). The pattern can't contain '{' or '}'. Nepali output goes through to_chars_*.
template <> struct std::formatter<nepdate::date, char> {
    std::string_view pattern;

    /// Takes everything up to the closing '}' as the pattern; an empty one means "YYYY/MM/DD".
    constexpr std::format_parse_context::iterator parse(std::format_parse_context& ctx) {
        // NOLINTBEGIN(readability-qualified-auto): the iterator is a class on MSVC and libc++ but a const char* on libstdc++
        auto it = ctx.begin();
        const auto start = it;
        // NOLINTEND(readability-qualified-auto)
        while (it != ctx.end() && *it != '}') {
            ++it;
        }
        pattern = std::string_view(start, it);
        return it;
    }

    template <class FormatContext>
    typename FormatContext::iterator format(nepdate::date d, FormatContext& ctx) const {
        nepdate::detail::iterator_sink<typename FormatContext::iterator> sink{ctx.out()};
        nepdate::detail::write_pattern(sink, d, pattern, nepdate::lang::en);
        sink.flush();
        return sink.out;
    }
};
#endif

#endif // NEPDATE_FORMAT_HPP
