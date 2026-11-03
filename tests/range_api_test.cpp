// The range vectors: construction, bounds, set operations, splits, day helpers and string forms.

// NOLINTBEGIN(bugprone-unchecked-optional-access): REQUIRE guards every optional access; the
// syntactic check cannot see through the Catch2 macro.
#include "vectors.hpp"

#include <nepdate/nepdate.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using nepdate::date;
using nepdate::date_order;
using nepdate::date_range;
using nepdate::lang;
using nepdate::make_date;
using nepdate::separator;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;

namespace {

date ymd(const std::string& text) {
    const auto v = parse_ymd(text);
    return make_date(v.year, v.month, v.day).value();
}

std::optional<date> try_ymd(const std::string& text) {
    const auto v = parse_ymd(text);
    const auto d = make_date(v.year, v.month, v.day);
    if (!d) {
        return std::nullopt;
    }
    return *d;
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

std::vector<std::string> split_comma(std::string_view text) {
    const std::size_t comma = text.find(',');
    if (comma == std::string_view::npos) {
        return {std::string(text)};
    }
    return {std::string(text.substr(0, comma)), std::string(text.substr(comma + 1))};
}

date_range parse_range(const std::string& text) {
    const std::size_t dot = text.find("..");
    return date_range::make(ymd(text.substr(0, dot)), ymd(text.substr(dot + 2))).value();
}

std::vector<date_range> parse_ranges(std::string_view text) {
    std::vector<date_range> out;
    for (const std::string& piece : split_list(text)) {
        out.push_back(parse_range(piece));
    }
    return out;
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one branch per range operation
TEST_CASE("csharp-golden/ranges.tsv: construction, bounds and set operations", "[vectors][range]") {
    const auto t = read_vectors("csharp-golden/ranges.tsv", '\t');
    REQUIRE(t.rows.size() == 54);
    const std::size_t start = t.column("start");
    const std::size_t end = t.column("end");
    const std::size_t operation = t.column("operation");
    const std::size_t argument = t.column("argument");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[operation] << " " << row[start] << ".." << row[end]);
        if (row[operation] == "construct") {
            const std::optional<date> first = try_ymd(row[start]);
            const std::optional<date> last = try_ymd(row[end]);
            const auto made = first.has_value() && last.has_value()
                                  ? date_range::make(*first, *last)
                                  : std::nullopt;
            if (row[result] == "ERR") {
                CHECK(!made.has_value());
            } else {
                REQUIRE(made.has_value());
                CHECK(*made == parse_range(row[result]));
            }
            continue;
        }
        const date_range r = parse_range(row[start] + ".." + row[end]);
        if (row[operation] == "length") {
            CHECK(r.size() == static_cast<std::size_t>(std::stoi(row[result])));
        } else if (row[operation] == "contains_date") {
            CHECK(r.contains(ymd(row[argument])) == (row[result] == "true"));
        } else if (row[operation] == "contains_range") {
            CHECK(r.contains(parse_range(row[argument])) == (row[result] == "true"));
        } else if (row[operation] == "overlaps") {
            CHECK(r.overlaps(parse_range(row[argument])) == (row[result] == "true"));
        } else if (row[operation] == "is_adjacent_to") {
            CHECK(r.is_adjacent_to(parse_range(row[argument])) == (row[result] == "true"));
        } else if (row[operation] == "intersect") {
            const auto got = r.intersect(parse_range(row[argument]));
            if (row[result] == "none") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == parse_range(row[result]));
            }
        } else if (row[operation] == "union") {
            CHECK(r.union_with(parse_range(row[argument])) == parse_range(row[result]));
        } else if (row[operation] == "except") {
            const auto got = r.subtract(parse_range(row[argument]));
            const std::vector<date_range> expected =
                row[result] == "none" ? std::vector<date_range>{} : parse_ranges(row[result]);
            std::vector<date_range> pieces;
            for (const date_range piece : got) {
                pieces.push_back(piece);
            }
            CHECK(pieces == expected);
        } else if (row[operation] == "equals") {
            CHECK((r == parse_range(row[argument])) == (row[result] == "true"));
        } else if (row[operation] == "to_string") {
            CHECK(nepdate::to_string(r) == row[result]);
        } else if (row[operation] == "enumerate") {
            std::vector<date> days;
            for (const date d : r) {
                days.push_back(d);
            }
            const std::vector<std::string> expected = split_list(row[result]);
            REQUIRE(days.size() == expected.size());
            for (std::size_t i = 0; i < expected.size(); ++i) {
                CHECK(days[i] == ymd(expected[i]));
            }
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one branch per factory
TEST_CASE("csharp-golden/range-factories.tsv", "[vectors][range]") {
    const auto t = read_vectors("csharp-golden/range-factories.tsv", '\t');
    REQUIRE(t.rows.size() == 27);
    const std::size_t factory = t.column("factory");
    const std::size_t argument = t.column("argument");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[factory] << " " << row[argument]);
        const std::vector<std::string> parts = split_comma(row[argument]);
        if (row[factory] == "single_day") {
            const std::optional<date> d = try_ymd(row[argument]);
            if (row[result] == "ERR") {
                CHECK(!d.has_value());
            } else {
                REQUIRE(d.has_value());
                CHECK(date_range::single(*d) == parse_range(row[result]));
            }
        } else if (row[factory] == "from_day_count") {
            const std::optional<date> start = try_ymd(parts[0]);
            const auto got = start.has_value()
                                 ? date_range::from_day_count(*start, std::stoi(parts[1]))
                                 : nepdate::result<date_range>{nepdate::errc::out_of_range};
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == parse_range(row[result]));
            }
        } else if (row[factory] == "for_month") {
            const auto got = date_range::of_month(std::stoi(parts[0]),
                                                  static_cast<nepdate::month>(std::stoi(parts[1])));
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == parse_range(row[result]));
            }
        } else if (row[factory] == "for_calendar_year") {
            const auto got = date_range::of_year(std::stoi(row[argument]));
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == parse_range(row[result]));
            }
        } else if (row[factory] == "for_fiscal_year") {
            const auto got = nepdate::fiscal_year{std::stoi(row[argument])}.range();
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
            } else {
                REQUIRE(got.has_value());
                CHECK(*got == parse_range(row[result]));
            }
        } else if (row[factory] == "construct") {
            const std::optional<date> first = try_ymd(parts[0]);
            const std::optional<date> last = try_ymd(parts[1]);
            const auto made = first.has_value() && last.has_value()
                                  ? date_range::make(*first, *last)
                                  : std::nullopt;
            if (row[result] == "ERR") {
                CHECK(!made.has_value());
            } else {
                REQUIRE(made.has_value());
                CHECK(*made == parse_range(row[result]));
            }
        }
    }
}

TEST_CASE("csharp-golden/range-splits.tsv: month and fiscal-quarter pieces", "[vectors][range]") {
    const auto t = read_vectors("csharp-golden/range-splits.tsv", '\t');
    REQUIRE(t.rows.size() == 9);
    const std::size_t start = t.column("start");
    const std::size_t end = t.column("end");
    const std::size_t operation = t.column("operation");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[operation] << " " << row[start] << ".." << row[end]);
        const date_range r = parse_range(row[start] + ".." + row[end]);
        const std::vector<date_range> got =
            row[operation] == "split_by_month" ? r.split_by_month() : r.split_by_fiscal_quarter();
        CHECK(got == parse_ranges(row[result]));
    }
}

TEST_CASE("csharp-golden/range-days.tsv: interval, working days and weekends", "[vectors][range]") {
    const auto t = read_vectors("csharp-golden/range-days.tsv", '\t');
    REQUIRE(t.rows.size() == 18);
    const std::size_t start = t.column("start");
    const std::size_t end = t.column("end");
    const std::size_t operation = t.column("operation");
    const std::size_t argument = t.column("argument");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[operation] << " " << row[start] << ".." << row[end]);
        const date_range r = parse_range(row[start] + ".." + row[end]);
        std::vector<date> days;
        if (row[operation] == "dates_with_interval") {
            const auto got = r.dates_with_interval(std::stoi(row[argument]));
            if (row[result] == "ERR") {
                CHECK(!got.has_value());
                continue;
            }
            REQUIRE(got.has_value());
            for (const date d : *got) {
                days.push_back(d);
            }
        } else if (row[operation] == "working_days") {
            const auto view = r.working_days(row[argument] == "true");
            for (const date d : view) {
                days.push_back(d);
            }
        } else if (row[operation] == "weekend_days") {
            const auto view = r.weekend_days(row[argument] == "true");
            for (const date d : view) {
                days.push_back(d);
            }
        } else if (row[operation] == "enumerate") {
            for (const date d : r) {
                days.push_back(d);
            }
        }
        const std::vector<std::string> expected =
            row[result] == "none" ? std::vector<std::string>{} : split_list(row[result]);
        REQUIRE(days.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(days[i] == ymd(expected[i]));
        }
    }
}

TEST_CASE("csharp-golden/range-format.tsv: default and short strings", "[vectors][range]") {
    const auto t = read_vectors("csharp-golden/range-format.tsv", '\t');
    REQUIRE(t.rows.size() == 18);
    const std::size_t start = t.column("start");
    const std::size_t end = t.column("end");
    const std::size_t kind = t.column("kind");
    const std::size_t order = t.column("order");
    const std::size_t separator_column = t.column("separator");
    const std::size_t language = t.column("lang");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    for (const auto& row : t.rows) {
        INFO(row[kind] << " " << row[start] << ".." << row[end]);
        const std::optional<date> first = try_ymd(row[start]);
        const std::optional<date> last = try_ymd(row[end]);
        if (row[result] == "ERR") {
            CHECK(!(first.has_value() && last.has_value() &&
                    date_range::make(*first, *last).has_value()));
            continue;
        }
        REQUIRE(first.has_value());
        REQUIRE(last.has_value());
        const date_range r = date_range::make(*first, *last).value();
        if (row[kind] == "default") {
            CHECK(nepdate::to_string(r) == row[result]);
        } else {
            CHECK(nepdate::format_range(
                      r, {order_of(row[order]), separator_of(row[separator_column]), true},
                      lang_of(row[language])) == row[result]);
        }
    }
}
// NOLINTEND(bugprone-unchecked-optional-access)
