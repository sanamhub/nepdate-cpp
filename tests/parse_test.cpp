// C3-05 and C3-06: strict and lenient parsing (PARSING sections 1 and 2) against
// shared/spec/vectors/parse.tsv. This file is UTF-8.

#include "vectors.hpp"

#include <nepdate/date.hpp>
#include <nepdate/format.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
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

TEST_CASE("lenient parse accepts the PARSING section 2 forms", "[parse]") {
    const date expected = make_date(2080, 4, 15).value();
    for (const char* s :
         {"15 Shrawan 2080", "Shrawan 15, 2080", "2080 Shrawan 15", "15 Srawan 2080",
          "15 SHRAWAN 2080", "Shr 15 2080", "15 साउन 2080", "१५ श्रावण २०८०", "15 साउन, 2080 गते",
          "2080/04/15 BS", "2080/04/15 B.S.", "B.S. 2080 Shrawan 15"}) {
        INFO(s);
        CHECK(nepdate::parse_lenient(s) == result<date>{expected});
    }
    CHECK(nepdate::parse_lenient("1 Baishakh 2081") == make_date(2081, 1, 1));
    CHECK(nepdate::parse_lenient("Baisakh 1, 2081") == make_date(2081, 1, 1));
    CHECK(nepdate::parse_lenient("30 Chaitra 2080") == make_date(2080, 12, 30));
    CHECK(nepdate::parse_lenient("15/05/2080") == make_date(2080, 5, 15));
    CHECK(nepdate::parse_lenient("15.04.2080") == make_date(2080, 4, 15));
    CHECK(nepdate::parse_lenient("२०८०-०४-३२ बि.सं.") == make_date(2080, 4, 32));
    CHECK(nepdate::parse_lenient("2081/04/32") == make_date(2081, 4, 32));
}

TEST_CASE("lenient parse errors", "[parse]") {
    CHECK(nepdate::parse_lenient("").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("   ").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("2080/05").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("2080").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("2080/05/15/99").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("abc/def/ghi").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("not a date").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("Shrawan Bhadra 2080").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("15 Foo 2080").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("2080.B.S. Shrawan 15").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("Shr.15 2080").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("15.Shrawan.2080").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("٢٠٨٠/٠٥/١٥").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("2080 Shrawan ²").error() == errc::unrecognized);
    CHECK(nepdate::parse_lenient("12345/01/01").error() == errc::number_too_long);
    CHECK(nepdate::parse_lenient("80/05/15").error() == errc::ambiguous);
    CHECK(nepdate::parse_lenient("10/05/20").error() == errc::ambiguous);
    CHECK(nepdate::parse_lenient("15-04-080").error() == errc::ambiguous);
    CHECK(nepdate::parse_lenient("15 shrawan 80").error() == errc::ambiguous);
    CHECK(nepdate::parse_lenient("2080/13/01").error() == errc::invalid_month);
    CHECK(nepdate::parse_lenient("2080/05/40").error() == errc::invalid_day);
    CHECK(nepdate::parse_lenient("1900/12/30").error() == errc::out_of_range);
    CHECK(nepdate::parse_lenient("2200/01/01").error() == errc::out_of_range);
    CHECK(parse("").error() == errc::wrong_group_count);
}

TEST_CASE("parse.tsv: strict column", "[parse][vectors]") {
    const auto t = read_vectors("parse.tsv", '\t');
    REQUIRE(t.rows.size() == 67);
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
    CHECK(checked == 67);
}

TEST_CASE("parse.tsv: lenient column", "[parse][vectors]") {
    const auto t = read_vectors("parse.tsv", '\t');
    REQUIRE(t.rows.size() == 67);
    const std::size_t input = t.column("input");
    const std::size_t lenient = t.column("lenient");
    REQUIRE(lenient < t.header.size());
    std::size_t checked = 0;
    for (const auto& row : t.rows) {
        INFO("input: [" << row[input] << "]");
        CHECK(actual_text(nepdate::parse_lenient(row[input])) == expected_text(row[lenient]));
        ++checked;
    }
    CHECK(checked == 67);
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

TEST_CASE("the YYYY<sep>MM<sep>DD fast path gives the same result as the general loop", "[parse]") {
    // A leading space makes the input 11 bytes, so it skips the fast path, and the loop ignores
    // leading separators: parse(" " + s) is the loop's answer for s.
    constexpr std::array<std::string_view, 7> years = {"0000", "1900", "1901", "2081",
                                                       "2199", "2200", "9999"};
    constexpr std::string_view seps = "-/._| ";
    std::size_t checked = 0;
    std::size_t mismatches = 0;
    for (const std::string_view y : years) {
        for (int m = 0; m < 100; ++m) {
            for (int d = 0; d < 100; d += 3) {
                for (const char sep : seps) {
                    std::string s(y);
                    s += sep;
                    s += static_cast<char>('0' + (m / 10));
                    s += static_cast<char>('0' + (m % 10));
                    s += sep;
                    s += static_cast<char>('0' + (d / 10));
                    s += static_cast<char>('0' + (d % 10));
                    mismatches += parse(s) == parse(" " + s) ? 0U : 1U;
                    ++checked;
                }
            }
        }
    }
    CHECK(checked == std::size_t{7U} * 100U * 34U * 6U);
    CHECK(mismatches == 0);
    // Backslash separators, and near misses that must not take the fast path.
    CHECK(parse("2081\\04\\15") == make_date(2081, 4, 15));
    CHECK(parse("2081/04/1x").error() == errc::invalid_character);
    CHECK(parse("2081:04/15").error() == errc::invalid_character);
    CHECK(parse("20814/0/15").error() == errc::number_too_long);
}
