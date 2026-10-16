// The name lists in tools/codegen/names.hpp equal the tables of
// shared/spec/FORMATTING.md section 1 and PARSING.md sections 2 and 3, and the rendered header is
// ASCII.

#include "names.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::StartsWith;
namespace cg = nepdate::codegen;

namespace {

std::vector<std::string> read_lines(const std::string& relative) {
    std::ifstream in(std::string(NEPDATE_SHARED_DIR) + "/spec/" + relative, std::ios::binary);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::string trim(std::string_view s) {
    while (!s.empty() && s.front() == ' ') {
        s.remove_prefix(1);
    }
    while (!s.empty() && s.back() == ' ') {
        s.remove_suffix(1);
    }
    return std::string(s);
}

// The cells of a Markdown table row "| a | b |", trimmed and without ** markers.
std::vector<std::string> cells(const std::string& line) {
    std::vector<std::string> out;
    if (line.size() < 2 || line.front() != '|') {
        return out;
    }
    std::size_t start = 1;
    for (std::size_t end = line.find('|', start); end != std::string::npos;
         end = line.find('|', start)) {
        std::string cell = trim(std::string_view(line).substr(start, end - start));
        if (cell.size() >= 4 && cell.starts_with("**") && cell.ends_with("**")) {
            cell = cell.substr(2, cell.size() - 4);
        }
        out.push_back(cell);
        start = end + 1;
    }
    return out;
}

// Rows of a table whose first cell is a number and that have `width` cells.
std::vector<std::vector<std::string>> numbered_rows(const std::vector<std::string>& lines,
                                                    std::size_t width) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& line : lines) {
        auto row = cells(line);
        if (row.size() == width && !row[0].empty() &&
            std::all_of(row[0].begin(), row[0].end(),
                        [](char c) { return c >= '0' && c <= '9'; })) {
            rows.push_back(row);
        }
    }
    return rows;
}

// The `quoted` items of the first line containing marker and of the line after it (a list item
// can wrap onto the next line).
std::vector<std::string> backticked(const std::vector<std::string>& lines,
                                    std::string_view marker) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(marker) == std::string::npos) {
            continue;
        }
        std::string line = lines[i];
        if (i + 1 < lines.size() && !line.ends_with('.')) {
            line += ' ' + trim(lines[i + 1]);
        }
        std::size_t open = line.find('`');
        while (open != std::string::npos) {
            const std::size_t close = line.find('`', open + 1);
            if (close == std::string::npos) {
                break;
            }
            out.push_back(line.substr(open + 1, close - open - 1));
            open = line.find('`', close + 1);
        }
        break;
    }
    return out;
}

std::string ascii_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return s;
}

} // namespace

TEST_CASE("month names equal FORMATTING section 1", "[codegen][names]") {
    const auto rows = numbered_rows(read_lines("FORMATTING.md"), 4);
    REQUIRE(rows.size() == 12);
    for (std::size_t i = 0; i < 12; ++i) {
        CAPTURE(i);
        CHECK(rows[i][0] == std::to_string(i + 1));
        CHECK(rows[i][1] == cg::kMonthNamesEn.at(i));
        CHECK(rows[i][2] == cg::kMonthShortEn.at(i));
        CHECK(rows[i][3] == cg::kMonthNamesNe.at(i));
    }
}

TEST_CASE("weekday names equal FORMATTING section 1", "[codegen][names]") {
    const auto rows = numbered_rows(read_lines("FORMATTING.md"), 5);
    REQUIRE(rows.size() == 7);
    for (std::size_t i = 0; i < 7; ++i) {
        CAPTURE(i);
        CHECK(rows[i][0] == std::to_string(i));
        CHECK(rows[i][1] == cg::kWeekdayNamesEn.at(i));
        CHECK(rows[i][2] == cg::kWeekdayShortEn.at(i));
        CHECK(rows[i][3] == cg::kWeekdayNamesNe.at(i));
        CHECK(rows[i][4] == cg::kWeekdayShortNe.at(i));
    }
}

TEST_CASE("month spellings equal PARSING section 3", "[codegen][names]") {
    const auto rows = numbered_rows(read_lines("PARSING.md"), 2);
    REQUIRE(rows.size() == 12);
    for (std::size_t i = 0; i < 12; ++i) {
        CAPTURE(i);
        CHECK(rows[i][0] == std::to_string(i + 1));
        CHECK(rows[i][1] == cg::kMonthSpellingRows.at(i));
    }
}

TEST_CASE("era markers and dropped words equal PARSING section 2", "[codegen][names]") {
    const auto lines = read_lines("PARSING.md");
    auto eras = backticked(lines, "era markers:");
    for (auto& e : eras) {
        e = ascii_lower(e);
    }
    REQUIRE(eras.size() == cg::kEraMarkers.size());
    CHECK(std::equal(eras.begin(), eras.end(), cg::kEraMarkers.begin()));

    const auto dropped = backticked(lines, "Drop tokens `");
    REQUIRE(dropped.size() == cg::kDroppedWords.size());
    CHECK(std::equal(dropped.begin(), dropped.end(), cg::kDroppedWords.begin()));
}

TEST_CASE("the names header is ASCII, LF only and deterministic", "[codegen][names]") {
    const std::string first = cg::render_names_tables();
    CHECK(first == cg::render_names_tables());
    CHECK(std::none_of(first.begin(), first.end(),
                       [](char c) { return static_cast<unsigned char>(c) >= 0x80U || c == '\r'; }));
    CHECK_THAT(first, StartsWith("// Generated by tools/codegen"));
    // The 12 rows hold 82 spellings; baisakh and bhadra appear twice in their row.
    CHECK(first.find("std::array<month_spelling, 80>") != std::string::npos);
}
