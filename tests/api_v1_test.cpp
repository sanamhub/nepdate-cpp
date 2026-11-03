// The API v1 vectors that are not ranges: year length, day difference, the YYYYMMDD integer,
// ordered batch conversion, the canonical string form and the format matrix.

// NOLINTBEGIN(bugprone-unchecked-optional-access): REQUIRE guards every optional access; the
// syntactic check cannot see through the Catch2 macro.
#include "vectors.hpp"

#include <nepdate/nepdate.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using nepdate::date;
using nepdate::date_order;
using nepdate::date_range;
using nepdate::format_short;
using nepdate::from_ymd_int;
using nepdate::lang;
using nepdate::make_date;
using nepdate::separator;
using nepdate::short_format;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;

namespace {

date ymd(const std::string& text) {
    const auto v = parse_ymd(text);
    return make_date(v.year, v.month, v.day).value();
}

std::vector<std::string> split_list(std::string_view text) {
    std::vector<std::string> out;
    std::size_t start = 0;
    for (;;) {
        const std::size_t end = text.find(';', start);
        out.emplace_back(
            text.substr(start, end == std::string_view::npos ? text.size() - start : end - start));
        if (end == std::string_view::npos) {
            return out;
        }
        start = end + 1;
    }
}

std::chrono::sys_days gregorian_day(const std::string& text) {
    const auto v = parse_ymd(text);
    return std::chrono::sys_days{std::chrono::year{v.year} /
                                 std::chrono::month{static_cast<unsigned>(v.month)} /
                                 std::chrono::day{static_cast<unsigned>(v.day)}};
}

date_order order_of(std::string_view name) {
    if (name == "Ydm") {
        return date_order::ydm;
    }
    if (name == "Myd") {
        return date_order::myd;
    }
    if (name == "Mdy") {
        return date_order::mdy;
    }
    if (name == "Dym") {
        return date_order::dym;
    }
    if (name == "Dmy") {
        return date_order::dmy;
    }
    return date_order::ymd;
}

separator separator_of(std::string_view name) {
    if (name == "Backslash") {
        return separator::backslash;
    }
    if (name == "Dot") {
        return separator::dot;
    }
    if (name == "Underscore") {
        return separator::underscore;
    }
    if (name == "Dash") {
        return separator::dash;
    }
    if (name == "Space") {
        return separator::space;
    }
    return separator::slash;
}

lang lang_of(std::string_view name) {
    return name == "ne" ? lang::ne : lang::en;
}

} // namespace

TEST_CASE("year-length.tsv: every BS year length", "[vectors][year-length]") {
    const auto t = read_vectors("year-length.tsv", '\t');
    REQUIRE(t.rows.size() == 299);
    const std::size_t year = t.column("bs_year");
    const std::size_t length = t.column("year_length");
    REQUIRE(length < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[year]);
        const int y = std::stoi(row[year]);
        const date d = make_date(y, 1, 1).value();
        CHECK(d.year_length() == std::stoi(row[length]));
        CHECK(nepdate::date_range::of_year(y).value().size() ==
              static_cast<std::size_t>(std::stoi(row[length])));
    }
}

TEST_CASE("csharp-golden/day-diff.tsv: named direction and operator direction", "[vectors][diff]") {
    const auto t = read_vectors("csharp-golden/day-diff.tsv", '\t');
    REQUIRE(t.rows.size() == 16);
    const std::size_t a = t.column("a");
    const std::size_t b = t.column("b");
    const std::size_t days_until = t.column("days_until");
    const std::size_t operator_minus = t.column("operator_minus");
    REQUIRE(operator_minus < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[a] << " " << row[b]);
        const date left = ymd(row[a]);
        const date right = ymd(row[b]);
        CHECK(left.days_until(right) == std::stoi(row[days_until]));
        CHECK((left - right).total_days == std::stoi(row[operator_minus]));
        CHECK(left.diff(right).total_days == left.days_until(right));
    }
}

TEST_CASE("csharp-golden/ymd-int.tsv: the YYYYMMDD integer both ways", "[vectors][serial]") {
    const auto t = read_vectors("csharp-golden/ymd-int.tsv", '\t');
    REQUIRE(t.rows.size() == 14);
    const std::size_t kind = t.column("kind");
    const std::size_t input = t.column("input");
    const std::size_t ymd_column = t.column("ymd");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[kind] << " " << row[ymd_column]);
        if (row[kind] == "date") {
            const date d = ymd(row[input]);
            CHECK(d.to_ymd_int() == std::stoi(row[ymd_column]));
            const auto back = from_ymd_int(std::stoi(row[ymd_column]));
            REQUIRE(back.has_value());
            CHECK(*back == d);
        } else {
            const auto got = from_ymd_int(std::stoi(row[ymd_column]));
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == ymd(row[result]));
            }
        }
    }
    // The widest integer the type can hold is decoded before narrowing, so it can't wrap in.
    CHECK(!from_ymd_int(std::numeric_limits<int>::max()).has_value());
    CHECK(!from_ymd_int(std::numeric_limits<std::int64_t>::max()).has_value());
    CHECK(!from_ymd_int(std::numeric_limits<std::int64_t>::min()).has_value());
    CHECK(from_ymd_int(21991230).value() == date::max());
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one branch per bulk operation
TEST_CASE("csharp-golden/bulk.tsv: every operation keeps input order", "[vectors][bulk]") {
    const auto t = read_vectors("csharp-golden/bulk.tsv", '\t');
    REQUIRE(t.rows.size() == 6);
    const std::size_t operation = t.column("operation");
    const std::size_t input = t.column("input");
    const std::size_t output = t.column("output");
    REQUIRE(output < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[operation]);
        const std::vector<std::string> inputs = split_list(row[input]);
        if (row[operation] == "to_nepali_dates" || row[operation] == "batch_to_nepali_dates") {
            std::vector<std::chrono::sys_days> gregorian;
            gregorian.reserve(inputs.size());
            for (const std::string& text : inputs) {
                gregorian.push_back(gregorian_day(text));
            }
            const auto got = nepdate::to_nepali_dates(gregorian);
            if (row[output] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                const std::vector<std::string> expected = split_list(row[output]);
                REQUIRE(got->size() == expected.size());
                for (std::size_t i = 0; i < expected.size(); ++i) {
                    CHECK((*got)[i] == ymd(expected[i]));
                }
            }
        } else if (row[operation] == "to_english_dates_from_strings") {
            const auto got = nepdate::to_english_dates(inputs);
            if (row[output] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                const std::vector<std::string> expected = split_list(row[output]);
                REQUIRE(got->size() == expected.size());
                for (std::size_t i = 0; i < expected.size(); ++i) {
                    CHECK((*got)[i] == gregorian_day(expected[i]));
                }
            }
        } else {
            std::vector<date> dates;
            dates.reserve(inputs.size());
            for (const std::string& text : inputs) {
                dates.push_back(ymd(text));
            }
            const std::vector<std::chrono::sys_days> got = nepdate::to_english_dates(dates);
            const std::vector<std::string> expected = split_list(row[output]);
            REQUIRE(got.size() == expected.size());
            for (std::size_t i = 0; i < expected.size(); ++i) {
                CHECK(got[i] == gregorian_day(expected[i]));
            }
        }
    }
}

TEST_CASE("csharp-golden/serialization.tsv: the canonical string hook", "[vectors][serial]") {
    const auto t = read_vectors("csharp-golden/serialization.tsv", '\t');
    REQUIRE(t.rows.size() == 11);
    const std::size_t kind = t.column("kind");
    const std::size_t input = t.column("input");
    const std::size_t json = t.column("json");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[kind] << " " << row[input]);
        if (row[kind] == "write") {
            CHECK('"' + nepdate::to_iso_string(ymd(row[input])) + '"' == row[json]);
        } else if (row[kind] == "read") {
            if (row[input] == "null") {
                CHECK(!nepdate::try_parse(row[input]).has_value());
            } else {
                // The json field is literal JSON, quotes included.
                const std::string text = row[input].substr(1, row[input].size() - 2);
                const auto got = nepdate::try_parse(text);
                if (row[result] == "ERR") {
                    CHECK(!got.has_value());
                } else {
                    REQUIRE(got.has_value());
                    CHECK(*got == ymd(row[result]));
                }
            }
        } else {
            const date d = ymd(row[input]);
            CHECK('"' + nepdate::to_iso_string(d) + '"' == row[json]);
            const auto got = nepdate::try_parse(nepdate::to_iso_string(d));
            REQUIRE(got.has_value());
            CHECK(*got == ymd(row[result]));
        }
    }
}

TEST_CASE("a step at the type's maximum never overflows or wraps", "[range][edge]") {
    const date start = make_date(2081, 4, 10).value();
    const date_range r = nepdate::date_range::make(start, make_date(2081, 4, 20).value()).value();

    // The walk stops before the step that would pass the end, even at INT_MAX.
    const auto widest = r.dates_with_interval(std::numeric_limits<int>::max());
    REQUIRE(widest.has_value());
    CHECK(widest->size() == 1);
    CHECK(*widest->begin() == start);

    const auto second = r.dates_with_interval(std::numeric_limits<int>::max() - 1);
    REQUIRE(second.has_value());
    CHECK(second->size() == 1);

    const auto whole = r.dates_with_interval(11);
    REQUIRE(whole.has_value());
    CHECK(whole->size() == 1);

    // A day count at INT_MAX stops at the end instead of wrapping.
    CHECK(!nepdate::date_range::from_day_count(start, std::numeric_limits<int>::max()).has_value());
    CHECK(!nepdate::date_range::from_day_count(start, std::numeric_limits<int>::min()).has_value());
    CHECK(!start.add_days(std::numeric_limits<int>::max()).has_value());
    CHECK(!start.add_months(std::numeric_limits<int>::max()).has_value());
    CHECK(!start.add_years(std::numeric_limits<int>::max()).has_value());
}
// NOLINTEND(bugprone-unchecked-optional-access)

TEST_CASE("csharp-golden/format-matrix.tsv: orders, separators, pad and TryFormat",
          "[vectors][format]") {
    const auto t = read_vectors("csharp-golden/format-matrix.tsv", '\t');
    REQUIRE(t.rows.size() == 102);
    const std::size_t kind = t.column("kind");
    const std::size_t bs = t.column("bs");
    const std::size_t order = t.column("order");
    const std::size_t separator_column = t.column("separator");
    const std::size_t pad = t.column("pad");
    const std::size_t language = t.column("lang");
    const std::size_t format = t.column("format");
    const std::size_t capacity = t.column("capacity");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[kind] << " " << row[bs]);
        const date d = ymd(row[bs]);
        const lang l = lang_of(row[language]);
        if (row[kind] == "short") {
            const short_format f{order_of(row[order]), separator_of(row[separator_column]),
                                 row[pad] == "true"};
            CHECK(format_short(d, f, l) == row[result]);
        } else {
            const int size = std::stoi(row[capacity]);
            std::array<char, 64> buf{};
            const std::to_chars_result r =
                row[format] == "default"
                    ? nepdate::to_chars_short(buf.data(), buf.data() + size, d, short_format{}, l)
                    : nepdate::to_chars_pattern(buf.data(), buf.data() + size, d, row[format], l);
            if (row[result] == "ERR") {
                CHECK(r.ec == std::errc::value_too_large);
            } else {
                CHECK(r.ec == std::errc{});
                CHECK(std::string_view(buf.data(), static_cast<std::size_t>(r.ptr - buf.data())) ==
                      row[result]);
            }
        }
    }
}
