// C3-02: to_chars, to_chars_iso, to_chars_short, to_chars_long and the std::string functions
// (FORMATTING sections 2 to 4). This file is UTF-8.

#include "vectors.hpp"

#include <nepdate/date_range.hpp>
#include <nepdate/format.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>

using nepdate::date;
using nepdate::date_order;
using nepdate::lang;
using nepdate::long_format;
using nepdate::make_date;
using nepdate::separator;
using nepdate::short_format;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;

namespace {

date ymd(int y, int m, int d) {
    return make_date(y, m, d).value();
}

constexpr std::array<date_order, 6> kOrders = {date_order::ymd, date_order::ydm, date_order::myd,
                                               date_order::mdy, date_order::dym, date_order::dmy};
constexpr std::array<separator, 6> kSeparators = {separator::slash, separator::backslash,
                                                  separator::dot,   separator::underscore,
                                                  separator::dash,  separator::space};

// Byte guarding the end of a test buffer: a write past last changes it.
constexpr char kCanary = '#';

} // namespace

TEST_CASE("csharp-golden/dates.tsv: text columns", "[format][vectors]") {
    const auto t = read_vectors("csharp-golden/dates.tsv", '\t');
    REQUIRE(t.rows.size() == 1018);
    const std::size_t bs = t.column("bs");
    const std::size_t to_string = t.column("to_string");
    const std::size_t unicode = t.column("unicode");
    const std::size_t long_en = t.column("long_en");
    const std::size_t long_en_weekday = t.column("long_en_weekday");
    const std::size_t long_ne = t.column("long_ne");
    const std::size_t long_ne_weekday = t.column("long_ne_weekday");
    const std::size_t dmy_dash_nopad = t.column("dmy_dash_nopad");
    REQUIRE(dmy_dash_nopad < t.header.size());
    const long_format weekday{.weekday = true};
    std::size_t checked = 0;
    for (const auto& row : t.rows) {
        INFO(row[bs]);
        const auto b = parse_ymd(row[bs]);
        const date d = ymd(b.year, b.month, b.day);
        CHECK(nepdate::to_string(d) == row[to_string]);
        CHECK(nepdate::format_short(d, {}, lang::ne) == row[unicode]);
        CHECK(nepdate::format_long(d, {}, lang::en) == row[long_en]);
        CHECK(nepdate::format_long(d, weekday, lang::en) == row[long_en_weekday]);
        CHECK(nepdate::format_long(d, {}, lang::ne) == row[long_ne]);
        CHECK(nepdate::format_long(d, weekday, lang::ne) == row[long_ne_weekday]);
        CHECK(nepdate::format_short(d, {date_order::dmy, separator::dash, false}, lang::en) ==
              row[dmy_dash_nopad]);
        ++checked;
    }
    CHECK(checked == 1018);
}

TEST_CASE("FORMATTING section 3 examples", "[format]") {
    const date d = ymd(2080, 5, 15);
    CHECK(nepdate::format_short(d, {date_order::ymd, separator::dash, true}, lang::en) ==
          "2080-05-15");
    CHECK(nepdate::format_short(d, {date_order::dmy, separator::dash, false}, lang::en) ==
          "15-5-2080");
    CHECK(nepdate::format_short(d, {date_order::ymd, separator::slash, true}, lang::ne) ==
          "२०८०/०५/१५");
    CHECK(nepdate::format_short(d, {date_order::ydm, separator::backslash, true}, lang::en) ==
          "2080\\15\\05");
    CHECK(nepdate::format_short(d, {date_order::myd, separator::underscore, true}, lang::en) ==
          "05_2080_15");
    CHECK(nepdate::format_short(d, {date_order::mdy, separator::dot, false}, lang::en) ==
          "5.15.2080");
    CHECK(nepdate::format_short(d, {date_order::dym, separator::space, true}, lang::en) ==
          "15 2080 05");
}

TEST_CASE("FORMATTING section 4 table", "[format]") {
    const date d = ymd(2081, 4, 15);
    CHECK(nepdate::format_long(d, {}, lang::en) == "Shrawan 15, 2081");
    CHECK(nepdate::format_long(d, {}, lang::ne) == "साउन १५, २०८१");
    CHECK(nepdate::format_long(d, {.weekday = true}, lang::en) == "Tuesday, Shrawan 15, 2081");
    CHECK(nepdate::format_long(d, {.weekday = true}, lang::ne) == "मङ्गलवार, साउन १५, २०८१");
    CHECK(nepdate::format_long(ymd(1901, 1, 1), {}, lang::en) == "Baishakh 01, 1901");
    CHECK(nepdate::format_long(ymd(1901, 1, 1), {}, lang::ne) == "बैशाख ०१, १९०१");
    const long_format no_year_no_pad{.weekday = true, .year = false, .pad = false};
    CHECK(nepdate::format_long(ymd(2079, 2, 6), no_year_no_pad, lang::en) == "Friday, Jestha 6");
    CHECK(nepdate::format_long(ymd(2079, 2, 6), no_year_no_pad, lang::ne) == "शुक्रवार, जेठ ६");
}

TEST_CASE("to_string, to_iso_string and their to_chars forms", "[format]") {
    const date d = ymd(2081, 4, 15);
    CHECK(nepdate::to_string(d) == "2081/04/15");
    CHECK(nepdate::to_iso_string(d) == "2081-04-15");
    CHECK(nepdate::to_string(date::min()) == "1901/01/01");
    CHECK(nepdate::to_iso_string(date::max()) == "2199-12-30");
    std::array<char, 10> buf{};
    const auto r = nepdate::to_chars(buf.data(), buf.data() + buf.size(), d);
    CHECK(r.ec == std::errc{});
    CHECK(std::string_view(buf.data(), static_cast<std::size_t>(r.ptr - buf.data())) ==
          "2081/04/15");
}

TEST_CASE("every 2081 output fits the size bounds; one byte less fails without overrun",
          "[format]") {
    std::size_t longest_short = 0;
    std::size_t longest_long = 0;
    std::array<char, nepdate::max_long_size + 1> buf{};
    for (const date d : nepdate::date_range::of_year(2081).value()) {
        for (const lang l : {lang::en, lang::ne}) {
            for (const bool pad : {true, false}) {
                for (const date_order o : kOrders) {
                    for (const separator s : kSeparators) {
                        const short_format f{o, s, pad};
                        buf.fill(kCanary);
                        auto r = nepdate::to_chars_short(
                            buf.data(), buf.data() + nepdate::max_short_size, d, f, l);
                        REQUIRE(r.ec == std::errc{});
                        const auto n = static_cast<std::size_t>(r.ptr - buf.data());
                        longest_short = n > longest_short ? n : longest_short;
                        buf.fill(kCanary);
                        r = nepdate::to_chars_short(buf.data(), buf.data() + n - 1, d, f, l);
                        REQUIRE(r.ec == std::errc::value_too_large);
                        REQUIRE(r.ptr == buf.data() + n - 1);
                        REQUIRE(buf[n - 1] == kCanary);
                    }
                }
                for (const bool weekday : {true, false}) {
                    for (const bool year : {true, false}) {
                        const long_format f{weekday, year, pad};
                        buf.fill(kCanary);
                        auto r = nepdate::to_chars_long(
                            buf.data(), buf.data() + nepdate::max_long_size, d, f, l);
                        REQUIRE(r.ec == std::errc{});
                        const auto n = static_cast<std::size_t>(r.ptr - buf.data());
                        longest_long = n > longest_long ? n : longest_long;
                        buf.fill(kCanary);
                        r = nepdate::to_chars_long(buf.data(), buf.data() + n - 1, d, f, l);
                        REQUIRE(r.ec == std::errc::value_too_large);
                        REQUIRE(r.ptr == buf.data() + n - 1);
                        REQUIRE(buf[n - 1] == kCanary);
                    }
                }
            }
        }
    }
    // docs/API.md: at most 26 and 68 bytes are used.
    CHECK(longest_short == 26);
    CHECK(longest_long <= 68);
}

TEST_CASE("an empty buffer reports value_too_large", "[format]") {
    std::array<char, 1> buf{kCanary};
    const date d = ymd(2081, 4, 15);
    CHECK(nepdate::to_chars(buf.data(), buf.data(), d).ec == std::errc::value_too_large);
    CHECK(nepdate::to_chars_long(buf.data(), buf.data(), d, {}, lang::ne).ec ==
          std::errc::value_too_large);
    CHECK(buf[0] == kCanary);
}
