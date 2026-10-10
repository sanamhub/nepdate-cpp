// C3-05 and C3-06: strict and lenient parsing (PARSING sections 1 and 2) against
// shared/spec/vectors/parse.tsv. This file is UTF-8.

#include "vectors.hpp"

#include <nepdate/format.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

using nepdate::date;
using nepdate::errc;
using nepdate::make_date;
using nepdate::parse;
using nepdate::result;
using nepdate_test::read_vectors;

namespace {

// "ERR:WrongGroupCount" -> "wrong_group_count"; anything else unchanged.
std::string expected_text(const std::string& cell) {
    if (!cell.starts_with("ERR:")) {
        return cell;
    }
    std::string out;
    for (const char c : cell.substr(4)) {
        if (c >= 'A' && c <= 'Z') {
            if (!out.empty()) {
                out += '_';
            }
            out += static_cast<char>(c - 'A' + 'a');
        } else {
            out += c;
        }
    }
    return out;
}

// The date as "2080-05-15", or the error's errc name.
std::string actual_text(result<date> r) {
    return r ? nepdate::to_iso_string(*r) : std::string(nepdate::to_string(r.error()));
}

} // namespace

TEST_CASE("parse.tsv: strict column", "[parse][vectors]") {
    const auto t = read_vectors("parse.tsv", '\t');
    REQUIRE(t.rows.size() == 63);
    const std::size_t input = t.column("input");
    const std::size_t strict = t.column("strict");
    REQUIRE(strict < t.header.size());
    std::size_t checked = 0;
    for (const auto& row : t.rows) {
        // The empty input is a row whose only field is empty: split gives it 3 fields anyway.
        INFO("input: [" << row[input] << "]");
        CHECK(actual_text(parse(row[input])) == expected_text(row[strict]));
        ++checked;
    }
    CHECK(checked == 63);
}

TEST_CASE("PARSING section 1 examples", "[parse]") {
    const date expected = make_date(2080, 5, 15).value();
    for (const char* s :
         {"2080/05/15", "2080-5-15", "  2080 / 05 / 15  ", "/2080/05/15/", "2080।05।15",
          "२०८०/०५/१५", "2080\\05\\15", "2080|05|15", "2080_05_15", "2080.05.15", "२०80/0५/१5"}) {
        INFO(s);
        CHECK(parse(s) == result<date>{expected});
    }
}

TEST_CASE("strict parse errors", "[parse]") {
    CHECK(parse("").error() == errc::wrong_group_count);
    CHECK(parse("   ").error() == errc::wrong_group_count);
    CHECK(parse("2080/05").error() == errc::wrong_group_count);
    CHECK(parse("2080/05/15/9").error() == errc::wrong_group_count);
    CHECK(parse("12345/01/01").error() == errc::number_too_long);
    CHECK(parse("2080/05/00015").error() == errc::number_too_long);
    CHECK(parse("2080/05/0015").value() == make_date(2080, 5, 15).value());
    CHECK(parse("2080,05,15").error() == errc::invalid_character);
    CHECK(parse("2080\t05\t15").error() == errc::invalid_character);
    CHECK(parse("२०८०/०५/१५ ").value() == make_date(2080, 5, 15).value());
    CHECK(parse("2080/13/01").error() == errc::invalid_month);
    CHECK(parse("9999/99/99").error() == errc::out_of_range);
    // The first error found left to right wins.
    CHECK(parse("2080/05/15/x").error() == errc::invalid_character);
    CHECK(parse("1/2/3/45678").error() == errc::wrong_group_count);
}

TEST_CASE("a cut-off Devanagari sequence is invalid_character and reads nothing past the end",
          "[parse]") {
    // The view ends inside the sequence; the bytes after it are never read (ASan in CI).
    const std::string_view full = "2080/05/1\xE0\xA5\xA7";
    CHECK(parse(full).value() == make_date(2080, 5, 11).value());
    CHECK(parse(full.substr(0, full.size() - 1)).error() == errc::invalid_character);
    CHECK(parse(full.substr(0, full.size() - 2)).error() == errc::invalid_character);
    CHECK(parse("2080/05/1\xE0\xA4\xA7").error() == errc::invalid_character);
    CHECK(parse("2080/05/1\xFF").error() == errc::invalid_character);
}

TEST_CASE("parse works on u8 strings and at compile time", "[parse][constexpr]") {
    CHECK(parse(u8"२०८०/०५/१५").value() == make_date(2080, 5, 15).value());
    CHECK(parse(std::u8string_view{}).error() == errc::wrong_group_count);
    STATIC_REQUIRE(parse("2080/05/15").value() == make_date(2080, 5, 15).value());
    STATIC_REQUIRE(parse("2080/5/40").error() == errc::invalid_day);
}

TEST_CASE("to_chars then parse gives the same date for every 32nd day", "[parse]") {
    for (int n = 0; n <= 109211; n += 32) {
        const date d = date::min().add_days(n).value();
        CHECK(parse(nepdate::to_string(d)).value() == d);
        CHECK(parse(nepdate::format_short(d, {}, nepdate::lang::ne)).value() == d);
    }
}
