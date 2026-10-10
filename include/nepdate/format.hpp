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

/// Separator of a short format: '/', '\', '.', '_', '-' or ' '.
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
    char* p;
    char* last;
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

/// Returns "2081/04/15". Fits the small-string buffer of every standard library, so it doesn't
/// allocate.
[[nodiscard]] inline std::string to_string(date d) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars(buf.data(), buf.data() + buf.size(), d);
    return std::string(buf.data(), r.ptr);
}

/// Returns "2081-04-15". Allocates only the returned string.
[[nodiscard]] inline std::string to_iso_string(date d) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_iso(buf.data(), buf.data() + buf.size(), d);
    return std::string(buf.data(), r.ptr);
}

/// Returns what to_chars_short writes. Allocates only the returned string.
[[nodiscard]] inline std::string format_short(date d, short_format f, lang l) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_short(buf.data(), buf.data() + buf.size(), d, f, l);
    return std::string(buf.data(), r.ptr);
}

/// Returns what to_chars_long writes. Allocates only the returned string.
[[nodiscard]] inline std::string format_long(date d, long_format f, lang l) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result r = to_chars_long(buf.data(), buf.data() + buf.size(), d, f, l);
    return std::string(buf.data(), r.ptr);
}

} // namespace nepdate

#endif // NEPDATE_FORMAT_HPP
