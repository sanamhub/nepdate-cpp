// C2-04: fiscal_year, quarter_of and months_of (ALGORITHM section 8b, D-08).

#include "vectors.hpp"

#include <nepdate/fiscal_year.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <climits>
#include <cstddef>
#include <string>
#include <system_error>

using nepdate::date;
using nepdate::errc;
using nepdate::fiscal_year;
using nepdate::lang;
using nepdate::make_date;
using nepdate::month;
using nepdate::quarter;
using nepdate::result;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;

namespace {

constexpr date bs(int y, int m, int d) {
    return make_date(y, m, d).value();
}

result<date> expected(const std::string& cell) {
    if (cell == "ERR") {
        return errc::out_of_range;
    }
    const auto v = parse_ymd(cell);
    return make_date(v.year, v.month, v.day);
}

template <class T> result<date> first_of(const result<T>& r) {
    return r ? result<date>{r->first()} : result<date>{r.error()};
}

template <class T> result<date> last_of(const result<T>& r) {
    return r ? result<date>{r->last()} : result<date>{r.error()};
}

std::string label_into_buffer(fiscal_year fy, lang l) {
    std::array<char, 64> buffer{};
    const auto r = fy.to_chars_label(buffer.data(), buffer.data() + buffer.size(), l);
    CHECK(r.ec == std::errc{});
    return std::string(buffer.data(), r.ptr);
}

} // namespace

TEST_CASE("csharp-golden/dates.tsv: fy_start, fy_end, quarter_start, quarter_end",
          "[vectors][fiscal]") {
    const auto t = read_vectors("csharp-golden/dates.tsv", '\t');
    REQUIRE(t.rows.size() >= 1018);
    const std::size_t bs_col = t.column("bs");
    const std::size_t fy_start = t.column("fy_start");
    const std::size_t fy_end = t.column("fy_end");
    const std::size_t q_start = t.column("quarter_start");
    const std::size_t q_end = t.column("quarter_end");
    REQUIRE(q_end < t.header.size());
    int errors = 0;
    for (const auto& row : t.rows) {
        INFO(row[bs_col]);
        const auto b = parse_ymd(row[bs_col]);
        const date d = make_date(b.year, b.month, b.day).value();
        const fiscal_year fy = fiscal_year::of(d);
        CHECK(fy.start() == expected(row[fy_start]));
        CHECK(fy.end() == expected(row[fy_end]));
        // The quarter of the row's date in its own fiscal year.
        const auto q = fy.quarter_range(nepdate::quarter_of(d));
        CHECK(first_of(q) == expected(row[q_start]));
        CHECK(last_of(q) == expected(row[q_end]));
        if (q) {
            CHECK(q->contains(d));
        }
        errors += row[fy_start] == "ERR" || row[fy_end] == "ERR" ? 1 : 0;
    }
    // The ERR cells are compared, not skipped.
    CHECK(errors > 0);
}

TEST_CASE("labels in English and Nepali", "[fiscal]") {
    CHECK(fiscal_year{2082}.label(lang::en) == "2082/83");
    CHECK(fiscal_year{2082}.label(lang::ne) == "२०८२/८३");
    CHECK(fiscal_year{2099}.label(lang::en) == "2099/00");
    CHECK(fiscal_year{2100}.label(lang::en) == "2100/01");
    CHECK(fiscal_year{1900}.label(lang::ne) == "१९००/०१");
    CHECK(fiscal_year{5}.label(lang::en) == "5/06");
    CHECK(fiscal_year{-1}.label(lang::en) == "-1/00");
    CHECK(fiscal_year{-150}.label(lang::en) == "-150/49");
    CHECK(fiscal_year{INT_MAX}.label(lang::en) == "2147483647/48");
    CHECK(fiscal_year{INT_MIN}.label(lang::en) == "-2147483648/47");
    CHECK(label_into_buffer(fiscal_year{2082}, lang::ne) == "२०८२/८३");
}

TEST_CASE("to_chars_label reports a buffer that is too small", "[fiscal]") {
    std::array<char, 19> buffer{};
    const fiscal_year fy{2082};
    for (std::size_t size = 0; size < 7; ++size) {
        const auto r = fy.to_chars_label(buffer.data(), buffer.data() + size, lang::en);
        CHECK(r.ec == std::errc::value_too_large);
        CHECK(r.ptr == buffer.data() + size);
    }
    CHECK(fy.to_chars_label(buffer.data(), buffer.data() + 7, lang::en).ec == std::errc{});
    CHECK(fy.to_chars_label(buffer.data(), buffer.data() + 18, lang::ne).ec ==
          std::errc::value_too_large);
    CHECK(fy.to_chars_label(buffer.data(), buffer.data() + 19, lang::ne).ec == std::errc{});
    CHECK(fiscal_year{-1}.to_chars_label(buffer.data(), buffer.data(), lang::en).ec ==
          std::errc::value_too_large);
}

TEST_CASE("start and end outside the calendar are out_of_range", "[fiscal]") {
    CHECK(fiscal_year{1900}.start().error() == errc::out_of_range);
    CHECK(fiscal_year{1900}.end() == bs(1901, 3, 1).last_of_month());
    CHECK(fiscal_year{2199}.end().error() == errc::out_of_range);
    CHECK(fiscal_year{2199}.start() == bs(2199, 4, 1));
    CHECK(fiscal_year{1900}.range().error() == errc::out_of_range);
    CHECK(fiscal_year{2199}.range().error() == errc::out_of_range);
    CHECK(fiscal_year{INT_MAX}.end().error() == errc::out_of_range);
    CHECK(fiscal_year{INT_MIN}.end().error() == errc::out_of_range);
    CHECK(fiscal_year{INT_MAX}.quarter_range(quarter::q4).error() == errc::out_of_range);
    CHECK(fiscal_year{INT_MIN}.quarter_range(quarter::q1).error() == errc::out_of_range);
    CHECK(fiscal_year{1900}.quarter_range(quarter::q4)->first() == date::min());
    CHECK(fiscal_year{2081}.quarter_range(static_cast<quarter>(0)).error() == errc::out_of_range);
    CHECK(fiscal_year{2081}.quarter_range(static_cast<quarter>(5)).error() == errc::out_of_range);
}

TEST_CASE("of, range and quarters of FY 2081/82", "[fiscal]") {
    STATIC_REQUIRE(fiscal_year{2081}.start() == bs(2081, 4, 1));
    STATIC_REQUIRE(fiscal_year::of(bs(2081, 4, 1)) == fiscal_year{2081});
    STATIC_REQUIRE(fiscal_year::of(bs(2082, 3, 31)) == fiscal_year{2081});
    STATIC_REQUIRE(fiscal_year{2081} < fiscal_year{2082});
    const auto r = fiscal_year{2081}.range();
    REQUIRE(r.has_value());
    CHECK(r->first() == bs(2081, 4, 1));
    CHECK(r->last() == bs(2082, 3, 1).last_of_month());
    std::size_t days = 0;
    for (const quarter q : {quarter::q1, quarter::q2, quarter::q3, quarter::q4}) {
        days += fiscal_year{2081}.quarter_range(q)->size();
    }
    CHECK(days == r->size());
    CHECK(fiscal_year{2081}.quarter_range(quarter::q4)->first() == bs(2082, 1, 1));
}

TEST_CASE("quarter_of and months_of", "[fiscal]") {
    STATIC_REQUIRE(nepdate::months_of(quarter::q4) ==
                   std::array{month::baishakh, month::jestha, month::ashad});
    STATIC_REQUIRE(nepdate::months_of(quarter::q1) ==
                   std::array{month::shrawan, month::bhadra, month::ashoj});
    STATIC_REQUIRE(nepdate::months_of(quarter::q2) ==
                   std::array{month::kartik, month::mangsir, month::poush});
    STATIC_REQUIRE(nepdate::months_of(quarter::q3) ==
                   std::array{month::magh, month::falgun, month::chaitra});
    for (int m = 1; m <= 12; ++m) {
        INFO(m);
        const auto months = nepdate::months_of(nepdate::quarter_of(bs(2081, m, 1)));
        const auto want = static_cast<month>(m);
        CHECK((months[0] == want || months[1] == want || months[2] == want));
    }
}
