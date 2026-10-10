// C3-01: month and weekday names (FORMATTING section 1) and to_chars_number. This file is UTF-8;
// the library headers hold the same text as octal escapes.

#include <nepdate/format.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>

using nepdate::lang;
using nepdate::month;
using nepdate::month_name;
using nepdate::month_short_name;
using nepdate::weekday_name;
using nepdate::weekday_short_name;
namespace chr = std::chrono;

namespace {

std::string number(unsigned value, int width, lang l) {
    std::array<char, 64> buf{};
    const auto r = nepdate::to_chars_number(buf.data(), buf.data() + buf.size(), value, width, l);
    REQUIRE(r.ec == std::errc{});
    return std::string(buf.data(), r.ptr);
}

} // namespace

TEST_CASE("month names in both languages", "[names]") {
    CHECK(month_name(month::shrawan, lang::ne) == "साउन");
    CHECK(month_name(month::shrawan, lang::en) == "Shrawan");
    CHECK(month_name(month::baishakh, lang::ne) == "बैशाख");
    CHECK(month_name(month::chaitra, lang::en) == "Chaitra");
    CHECK(month_short_name(month::ashad, lang::en) == "Asa");
    CHECK(month_short_name(month::ashoj, lang::en) == "Aso");
    CHECK(month_short_name(month::shrawan, lang::en) == "Shr");
    for (unsigned m = 1; m <= 12; ++m) {
        CAPTURE(m);
        const auto mm = static_cast<month>(m);
        CHECK(month_short_name(mm, lang::ne) == month_name(mm, lang::ne));
        CHECK(month_short_name(mm, lang::en).size() == 3);
        if (m != 3 && m != 6) { // D-04: Asa and Aso are not prefixes of Ashad and Ashoj
            CHECK(month_name(mm, lang::en).starts_with(month_short_name(mm, lang::en)));
        }
    }
}

TEST_CASE("weekday names in both languages", "[names]") {
    CHECK(weekday_name(chr::Sunday, lang::en) == "Sunday");
    CHECK(weekday_name(chr::Tuesday, lang::ne) == "मङ्गलवार");
    CHECK(weekday_short_name(chr::Tuesday, lang::ne) == "मङ्गल");
    CHECK(weekday_short_name(chr::Saturday, lang::en) == "Sat");
    CHECK(weekday_name(chr::Thursday, lang::ne) == "बिहिवार");
}

TEST_CASE("names outside the enumerators are empty", "[names]") {
    CHECK(month_name(static_cast<month>(13), lang::en).empty());
    CHECK(month_name(static_cast<month>(0), lang::ne).empty());
    CHECK(month_short_name(static_cast<month>(13), lang::en).empty());
    CHECK(weekday_name(chr::weekday{9}, lang::en).empty());
    CHECK(weekday_short_name(chr::weekday{9}, lang::ne).empty());
}

TEST_CASE("to_chars_number writes ASCII and Devanagari digits", "[names]") {
    CHECK(number(2081, 1, lang::ne) == "२०८१");
    CHECK(number(2081, 1, lang::en) == "2081");
    CHECK(number(5, 2, lang::ne) == "०५");
    CHECK(number(5, 2, lang::en) == "05");
    CHECK(number(0, 1, lang::en) == "0");
    CHECK(number(0, 0, lang::ne) == "०");
    CHECK(number(4294967295U, 1, lang::en) == "4294967295");
    CHECK(number(1234567890U, 1, lang::ne) == "१२३४५६७८९०");
    CHECK(number(7, 5, lang::en) == "00007");
    CHECK(number(7, -3, lang::en) == "7");
    for (unsigned v = 0; v < 1000; ++v) {
        CHECK(number(v, 3, lang::en) == (v < 10 ? "00" : v < 100 ? "0" : "") + std::to_string(v));
    }
}

TEST_CASE("to_chars_number reports a short buffer and stays inside it", "[names]") {
    std::array<char, 8> buf{};
    buf.fill('#');
    // Two Devanagari digits need 6 bytes; give it 5.
    auto r = nepdate::to_chars_number(buf.data(), buf.data() + 5, 15, 1, lang::ne);
    CHECK(r.ec == std::errc::value_too_large);
    CHECK(r.ptr == buf.data() + 5);
    CHECK(buf[5] == '#');
    r = nepdate::to_chars_number(buf.data(), buf.data() + 3, 7, 4, lang::en);
    CHECK(r.ec == std::errc::value_too_large);
    CHECK(buf[3] == '#');
    // A width no buffer holds fails cleanly instead of overflowing.
    r = nepdate::to_chars_number(buf.data(), buf.data() + buf.size(), 1, 2147483647, lang::ne);
    CHECK(r.ec == std::errc::value_too_large);
    // Exactly enough room.
    r = nepdate::to_chars_number(buf.data(), buf.data() + 6, 15, 1, lang::ne);
    CHECK(r.ec == std::errc{});
    CHECK(r.ptr == buf.data() + 6);
}

TEST_CASE("names and to_chars_number work at compile time", "[names][constexpr]") {
    STATIC_REQUIRE(month_short_name(month::ashoj, lang::en) == "Aso");
    STATIC_REQUIRE(weekday_name(chr::Friday, lang::en) == "Friday");
    STATIC_REQUIRE([] {
        std::array<char, 4> buf{};
        const auto r =
            nepdate::to_chars_number(buf.data(), buf.data() + buf.size(), 81, 4, lang::en);
        return r.ec == std::errc{} && std::string_view(buf.data(), 4) == "0081";
    }());
}
