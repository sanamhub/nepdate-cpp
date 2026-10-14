// C4-02: the patro tables of ADR-0002 section 7 equal PATRO sections 2 and 3, and the generator
// rejects a tithi gap by naming the missing date.

#include "calendar.hpp"
#include "patro.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using Catch::Matchers::ContainsSubstring;
namespace cg = nepdate::codegen;

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.is_open());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

struct built {
    cg::calendar calendar;
    cg::tables core_tables;
    cg::patro_input input;
    cg::patro_tables tables;
};

built build_all() {
    const auto parsed = cg::parse_calendar(
        read_file(std::string(NEPDATE_SHARED_DIR) + "/data/calendar/bs-calendar.json"));
    REQUIRE(parsed.error.empty());
    const auto core = cg::build_tables(parsed.value);
    REQUIRE(core.error.empty());
    std::vector<std::string> jsons;
    for (int year = 2077; year <= 2084; ++year) {
        jsons.push_back(read_file(std::string(NEPDATE_SHARED_DIR) + "/data/patro/" +
                                  std::to_string(year) + ".json"));
    }
    const auto parsed_patro = cg::parse_patro(jsons);
    REQUIRE(parsed_patro.error.empty());
    const auto tables = cg::build_patro_tables(parsed_patro.value, core.value.month_start);
    REQUIRE(tables.error.empty());
    return built{parsed.value, core.value, parsed_patro.value, tables.value};
}

// Serial of a BS date through the month-start table (ALGORITHM section 4).
std::uint32_t serial(const built& b, int year, int month, int day) {
    const std::size_t i =
        static_cast<std::size_t>(year - 1901) * 12U + static_cast<std::size_t>(month - 1);
    return b.core_tables.month_start.at(i) + static_cast<std::uint32_t>(day) - 1U;
}

} // namespace

TEST_CASE("the patro tables have the PATRO section 3 counts", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    CHECK(t.first == 64285);
    CHECK(t.days == 2793);
    std::size_t tithi_days = 0;
    std::size_t event_days = 0;
    for (std::size_t i = 0; i < t.day_tithi.size(); ++i) {
        tithi_days += t.day_tithi[i] != 0xFFU ? 1U : 0U;
        event_days += t.day_event_start[i] < t.day_event_start[i + 1] ? 1U : 0U;
    }
    CHECK(tithi_days == 2190);
    CHECK(event_days == 1671);
    CHECK(t.events.size() == 3032);
    CHECK(t.holidays.size() == 294);
    CHECK(t.holiday_bits.size() == 44);
    CHECK(t.names.size() == 753);
    CHECK(t.name_offsets.size() == 753 * 2 + 1);
    CHECK(t.name_offsets.back() == t.name_blob.size());
}

TEST_CASE("the tithi range is exactly 2078-01-02..2083-12-30, contiguous", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    const std::uint32_t first = serial(b, 2078, 1, 2) - t.first;
    const std::uint32_t last = serial(b, 2083, 12, 30) - t.first;
    REQUIRE(last < t.days);
    CHECK(t.day_tithi[first] != 0xFFU);
    CHECK(t.day_tithi[last] != 0xFFU);
    CHECK((first == 0 || t.day_tithi[first - 1] == 0xFFU));
    CHECK(t.day_tithi[last + 1] == 0xFFU);
    std::size_t tithi_days = 0;
    for (std::uint32_t i = first; i <= last; ++i) {
        CHECK(t.day_tithi[i] < 32);
        tithi_days += 1U;
    }
    CHECK(tithi_days == 2190);
}

TEST_CASE("removing one tithi day fails and names the missing date", "[patro]") {
    const built b = build_all();
    cg::patro_input broken = b.input;
    // Drop the second tithi day, 2078-01-03.
    std::size_t seen = 0;
    for (std::size_t i = 0; i < broken.days.size(); ++i) {
        if (!broken.days[i].has_tithi) {
            continue;
        }
        ++seen;
        if (seen == 2) {
            broken.days.erase(broken.days.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
    const auto rebuilt = cg::build_patro_tables(broken, b.core_tables.month_start);
    REQUIRE_FALSE(rebuilt.error.empty());
    CHECK_THAT(rebuilt.error, ContainsSubstring("tithi gap at 2078-01-03"));
}

TEST_CASE("holidays are strictly ascending and each has its bit set", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    REQUIRE(t.holidays.size() == 294);
    std::uint32_t previous = 0;
    for (std::size_t i = 0; i < t.holidays.size(); ++i) {
        const std::uint32_t packed = t.holidays[i];
        if (i != 0) {
            CHECK(packed > previous);
        }
        previous = packed;
        const int year = static_cast<int>(packed >> 16U);
        const int month = static_cast<int>((packed >> 8U) & 0xFFU);
        const int day = static_cast<int>(packed & 0xFFU);
        const std::uint32_t offset = serial(b, year, month, day) - t.first;
        CHECK((t.holiday_bits[offset / 64U] & (1ULL << (offset % 64U))) != 0);
    }
}

TEST_CASE("day event slices are in range and the sentinel equals the event count", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    for (std::size_t i = 0; i < t.day_event_start.size() - 1; ++i) {
        CHECK(t.day_event_start[i] <= t.day_event_start[i + 1]);
    }
    CHECK(t.day_event_start.back() == t.events.size());
    for (const std::uint16_t index : t.events) {
        CHECK(index < t.names.size());
    }
}

TEST_CASE("every name resolves through the blob to non-empty text", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    for (std::size_t k = 0; k < t.names.size(); ++k) {
        CAPTURE(k);
        const std::uint32_t np = t.name_offsets[2 * k];
        const std::uint32_t en = t.name_offsets[2 * k + 1];
        const std::uint32_t end = t.name_offsets[2 * k + 2];
        CHECK(np < en);
        CHECK(en <= end);
        CHECK(end <= t.name_blob.size());
        REQUIRE(en <= t.name_blob.size());
        const std::string np_text = t.name_blob.substr(np, en - np);
        const std::string en_text = t.name_blob.substr(en, end - en);
        CHECK(np_text == t.names[k].first);
        CHECK(en_text == t.names[k].second);
    }
}

TEST_CASE("rendering is deterministic and names the PATRO section 4 anchors", "[patro]") {
    const built b = build_all();
    const std::string once = cg::render_patro_tables(b.tables, b.core_tables.month_start);
    const std::string twice = cg::render_patro_tables(b.tables, b.core_tables.month_start);
    CHECK(once == twice);
    // The readable Nepali is in comments; the strings themselves are octal escapes.
    CHECK_THAT(once, ContainsSubstring("Panchami"));
    CHECK_THAT(once, ContainsSubstring("पञ्चमी"));
    CHECK_THAT(once, ContainsSubstring("World Day Against Human Trafficking"));
    CHECK_THAT(once, ContainsSubstring("'\\340','\\244','\\252'"));
    const std::string config = cg::render_patro_config(b.core_tables.fingerprint);
    std::string fingerprint = "0x";
    constexpr std::string_view hex_digits = "0123456789abcdef";
    for (int shift = 60; shift >= 0; shift -= 4) {
        fingerprint +=
            hex_digits.at((b.core_tables.fingerprint >> static_cast<unsigned>(shift)) & 0xfU);
    }
    CHECK_THAT(config, ContainsSubstring(fingerprint));
}
