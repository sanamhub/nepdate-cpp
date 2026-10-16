// to_chars_pattern and format_pattern (FORMATTING section 5). This file is UTF-8.

#include "vectors.hpp"

#include <nepdate/date.hpp>
#include <nepdate/format.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>

using nepdate::date;
using nepdate::format_pattern;
using nepdate::lang;
using nepdate::make_date;
using nepdate_test::parse_ymd;
using nepdate_test::read_vectors;

namespace {

date ymd(int y, int m, int d) {
    return make_date(y, m, d).value();
}

// True when the pattern holds a run of exactly three 'M'.
bool has_mmm_run(std::string_view pattern) {
    std::size_t i = 0;
    while (i < pattern.size()) {
        std::size_t j = i;
        while (j < pattern.size() && pattern[j] == 'M') {
            ++j;
        }
        if (j - i == 3) {
            return true;
        }
        i = j == i ? i + 1 : j;
    }
    return false;
}

} // namespace

TEST_CASE("csharp-golden/format-pattern.tsv", "[format][vectors]") {
    const auto t = read_vectors("csharp-golden/format-pattern.tsv", '\t');
    REQUIRE(t.rows.size() == 451);
    const std::size_t bs = t.column("bs");
    const std::size_t pattern = t.column("pattern");
    const std::size_t result = t.column("result");
    REQUIRE(result < t.header.size());
    std::size_t with_mmm = 0;
    std::size_t skipped = 0;
    std::size_t checked = 0;
    for (const auto& row : t.rows) {
        INFO(row[bs] << " " << row[pattern]);
        const auto b = parse_ymd(row[bs]);
        const date d = ymd(b.year, b.month, b.day);
        if (row[pattern].find("MMM") != std::string::npos) {
            ++with_mmm;
        }
        // D-04: C# writes "Ash" for both Ashad and Ashoj; this port writes "Asa" and "Aso".
        if (has_mmm_run(row[pattern]) && (b.month == 3 || b.month == 6)) {
            ++skipped;
            const std::string got = format_pattern(d, row[pattern], lang::en);
            CHECK(got.find(b.month == 3 ? "Asa" : "Aso") != std::string::npos);
            continue;
        }
        CHECK(format_pattern(d, row[pattern], lang::en) == row[result]);
        ++checked;
    }
    CHECK(with_mmm == 82);
    CHECK(skipped <= with_mmm);
    CHECK(skipped == 7);
    CHECK(checked == 444);
}

// format_pattern sizes its string with count_sink when the output outruns the stack buffer, so the
// count must equal what span_sink then writes, for every token and both languages. Only very long
// patterns reach that path through the public API, so this drives the two sinks directly.
TEST_CASE("count_sink measures exactly what span_sink writes", "[format][vectors]") {
    const auto t = read_vectors("csharp-golden/format-pattern.tsv", '\t');
    REQUIRE(t.rows.size() == 451);
    const std::size_t bs = t.column("bs");
    const std::size_t pattern = t.column("pattern");
    std::array<char, 512> buf{};
    std::size_t checked = 0;
    for (const auto& row : t.rows) {
        const auto b = parse_ymd(row[bs]);
        const date d = ymd(b.year, b.month, b.day);
        for (const lang l : {lang::en, lang::ne}) {
            INFO(row[bs] << " " << row[pattern] << " " << static_cast<int>(l));
            nepdate::detail::count_sink count;
            nepdate::detail::write_pattern(count, d, row[pattern], l);
            nepdate::detail::span_sink sink{buf.data(), buf.data() + buf.size()};
            nepdate::detail::write_pattern(sink, d, row[pattern], l);
            REQUIRE(sink.ok);
            CHECK(count.n == static_cast<std::size_t>(sink.p - buf.data()));
            ++checked;
        }
    }
    CHECK(checked == 902);
}

TEST_CASE("FORMATTING section 5 examples", "[format]") {
    const date d = ymd(2081, 4, 15);
    CHECK(format_pattern(d, "yyyy-MM-dd", lang::en) == "2081-04-15");
    CHECK(format_pattern(d, "M/d/yyyy", lang::en) == "4/15/2081");
    CHECK(format_pattern(d, "yyyy'BS'", lang::en) == "2081BS");
    CHECK(format_pattern(d, R"(yyyy\M\M-\d\d)", lang::en) == "2081MM-dd");
    CHECK(format_pattern(d, "MMM yyyy", lang::en) == "Shr 2081");
    CHECK(format_pattern(d, "dd.MM.yy", lang::en) == "15.04.81");
}

TEST_CASE("standard patterns", "[format]") {
    const date d = ymd(2081, 4, 15);
    for (const char* p : {"", "G", "g", "d"}) {
        CHECK(format_pattern(d, p, lang::en) == "2081/04/15");
    }
    CHECK(format_pattern(d, "d", lang::ne) == "२०८१/०४/१५");
    CHECK(format_pattern(d, "D", lang::en) == "Shrawan 15, 2081");
    CHECK(format_pattern(d, "D", lang::ne) == "साउन १५, २०८१");
    CHECK(format_pattern(d, "s", lang::en) == "2081-04-15");
    CHECK(format_pattern(d, "s", lang::ne) == "२०८१-०४-१५");
    // Only a whole pattern is standard: "dd" is the day token.
    CHECK(format_pattern(d, "dd", lang::en) == "15");
}

TEST_CASE("token edge cases", "[format]") {
    const date d = ymd(2081, 4, 5);
    CHECK(format_pattern(d, "yyyy'BS", lang::en) == "2081BS");
    CHECK(format_pattern(d, "'", lang::en).empty());
    CHECK(format_pattern(d, "yyyy\\", lang::en) == "2081");
    CHECK(format_pattern(d, "\\", lang::en).empty());
    CHECK(format_pattern(d, "ddd", lang::en) == "5");
    CHECK(format_pattern(d, "dddd", lang::en) == "5");
    CHECK(format_pattern(d, "y", lang::en) == "81");
    CHECK(format_pattern(d, "yyy", lang::en) == "81");
    CHECK(format_pattern(d, "yyyyy", lang::en) == "2081");
    CHECK(format_pattern(d, "MMMMM", lang::en) == "Shrawan");
    CHECK(format_pattern(ymd(2081, 3, 1), "MMM", lang::en) == "Asa");
    CHECK(format_pattern(ymd(2081, 6, 1), "MMM", lang::en) == "Aso");
    CHECK(format_pattern(d, "MMM", lang::ne) == "साउन");
    CHECK(format_pattern(d, "dd MMMM yyyy गते", lang::ne) == "०५ साउन २०८१ गते");
    // Literal text is never localised (D-15).
    CHECK(format_pattern(d, "'2081' yyyy", lang::ne) == "2081 २०८१");
}

TEST_CASE("a long pattern is never truncated", "[format]") {
    const date d = ymd(2081, 4, 15);
    std::string pattern;
    std::string expected;
    for (int i = 0; i < 40; ++i) {
        pattern += "MMMM yyyy ";
        expected += "साउन २०८१ ";
    }
    CHECK(format_pattern(d, pattern, lang::ne) == expected);
    std::array<char, 16> buf{};
    const auto r =
        nepdate::to_chars_pattern(buf.data(), buf.data() + buf.size(), d, pattern, lang::ne);
    CHECK(r.ec == std::errc::value_too_large);
    CHECK(r.ptr == buf.data() + buf.size());
}

TEST_CASE("to_chars_pattern works at compile time", "[format][constexpr]") {
    STATIC_REQUIRE([] {
        constexpr date d = make_date(2081, 4, 15).value();
        std::array<char, 32> buf{};
        const auto r = nepdate::to_chars_pattern(buf.data(), buf.data() + buf.size(), d,
                                                 "MMM d, yyyy", lang::en);
        return std::string_view(buf.data(), static_cast<std::size_t>(r.ptr - buf.data())) ==
               "Shr 15, 2081";
    }());
}
