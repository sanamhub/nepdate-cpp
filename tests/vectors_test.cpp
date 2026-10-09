// C1-06: the shared vectors and the spec cross-check (ALGORITHM §2).

#include "vectors.hpp"

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

using nepdate::date;
using nepdate::from_gregorian;
using nepdate::make_date;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;
using nepdate_test::weekday_index;

namespace {

std::chrono::year_month_day to_ymd(const nepdate_test::ymd& v) {
    return std::chrono::year_month_day{std::chrono::year{v.year},
                                       std::chrono::month{static_cast<unsigned>(v.month)},
                                       std::chrono::day{static_cast<unsigned>(v.day)}};
}

} // namespace

TEST_CASE("month-boundaries.csv: every first and last day of a month", "[vectors]") {
    const auto t = read_vectors("month-boundaries.csv", ',');
    REQUIRE(t.rows.size() >= 7176);
    const std::size_t bs = t.column("bs");
    const std::size_t ad = t.column("ad");
    const std::size_t weekday = t.column("weekday");
    const std::size_t length = t.column("month_length");
    REQUIRE(length < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[bs]);
        const auto b = parse_ymd(row[bs]);
        const auto a = parse_ymd(row[ad]);
        const auto d = make_date(b.year, b.month, b.day);
        REQUIRE(d.has_value());
        CHECK(d->to_year_month_day() == to_ymd(a));
        CHECK(from_gregorian(a.year, a.month, a.day) == d);
        CHECK(d->weekday().c_encoding() == weekday_index(row[weekday]));
        CHECK(d->month_length() == std::stoi(row[length]));
    }
}

TEST_CASE("csharp-golden/dates.tsv: ad, weekday, day_of_year, month_length", "[vectors]") {
    const auto t = read_vectors("csharp-golden/dates.tsv", '\t');
    REQUIRE(t.rows.size() >= 1018);
    const std::size_t bs = t.column("bs");
    const std::size_t ad = t.column("ad");
    const std::size_t weekday = t.column("weekday");
    const std::size_t day_of_year = t.column("day_of_year");
    const std::size_t length = t.column("month_length");
    REQUIRE(length < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[bs]);
        const auto b = parse_ymd(row[bs]);
        const auto d = make_date(b.year, b.month, b.day);
        REQUIRE(d.has_value());
        CHECK(d->to_year_month_day() == to_ymd(parse_ymd(row[ad])));
        CHECK(d->weekday().c_encoding() == weekday_index(row[weekday]));
        CHECK(d->day_of_year() == std::stoi(row[day_of_year]));
        CHECK(d->month_length() == std::stoi(row[length]));
    }
}

TEST_CASE("PACKED and YEAR_START from the vectors agree with the tables", "[vectors][spec]") {
    // LEN[y][m] from the month_length column, independent of the generated tables.
    constexpr int years = 299;
    std::array<std::array<int, 12>, years> len{};
    const auto t = read_vectors("month-boundaries.csv", ',');
    REQUIRE(t.rows.size() >= 7176);
    const std::size_t bs = t.column("bs");
    const std::size_t length = t.column("month_length");
    for (const auto& row : t.rows) {
        const auto b = parse_ymd(row[bs]);
        len.at(static_cast<std::size_t>(b.year - 1901)).at(static_cast<std::size_t>(b.month - 1)) =
            std::stoi(row[length]);
    }

    // ALGORITHM section 2.
    std::array<std::uint32_t, years> packed{};
    std::array<std::uint32_t, years + 1> year_start{};
    for (std::size_t i = 0; i < years; ++i) {
        std::uint32_t sum = 0;
        for (std::size_t m = 0; m < 12; ++m) {
            const int l = len.at(i).at(m);
            REQUIRE(l >= 29);
            REQUIRE(l <= 32);
            packed.at(i) |= static_cast<std::uint32_t>(l - 29) << (2 * m);
            sum += static_cast<std::uint32_t>(l);
        }
        year_start.at(i + 1) = year_start.at(i) + sum;
    }
    CHECK(year_start[years] == 109212);

    for (std::size_t i = 0; i < years; ++i) {
        const int y = 1901 + static_cast<int>(i);
        CHECK(nepdate::detail::kMonthStart.at(12 * i) == year_start.at(i));
        for (std::size_t m = 0; m < 12; ++m) {
            const int from_packed = 29 + static_cast<int>((packed.at(i) >> (2 * m)) & 3U);
            CHECK(make_date(y, static_cast<int>(m) + 1, 1)->month_length() == from_packed);
        }
    }
}
