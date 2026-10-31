// The patro tables of ADR-0002 section 7 equal PATRO sections 2 and 3 and match the counts in the
// hub's generated summary. The generator rejects a tithi gap by naming the missing date, and the
// summary check names a wrong field with both values.

#include "calendar.hpp"
#include "patro.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::ContainsSubstring;
namespace cg = nepdate::codegen;

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// The hub summary every port vendors; the tables must agree with every field.
cg::patro_summary read_summary() {
    const auto parsed = cg::parse_patro_summary(
        read_file(std::string(NEPDATE_SHARED_DIR) + "/spec/vectors/patro-summary.json"));
    REQUIRE(parsed.error.empty());
    return parsed.value;
}

struct built {
    cg::calendar calendar;
    cg::tables core_tables;
    cg::patro_input input;
    cg::patro_tables tables;
};

built build_all() {
    const cg::patro_summary summary = read_summary();
    const auto parsed = cg::parse_calendar(
        read_file(std::string(NEPDATE_SHARED_DIR) + "/data/calendar/bs-calendar.json"));
    REQUIRE(parsed.error.empty());
    const auto core = cg::build_tables(parsed.value);
    REQUIRE(core.error.empty());
    std::vector<std::string> jsons;
    for (int year = summary.first_year; year <= summary.last_year; ++year) {
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

std::string date_of_serial(const built& b, std::uint32_t serial_value) {
    std::size_t i = 0;
    while (b.core_tables.month_start[i + 1] <= serial_value) {
        ++i;
    }
    const int year = 1901 + static_cast<int>(i) / 12;
    const int month = static_cast<int>(i % 12U) + 1;
    const int day = static_cast<int>(serial_value - b.core_tables.month_start[i]) + 1;
    const auto pad = [](int value) {
        const std::string text = std::to_string(value);
        return text.size() < 2 ? "0" + text : text;
    };
    return std::to_string(year) + "-" + pad(month) + "-" + pad(day);
}

struct tithi_stats {
    std::uint32_t first_serial = 0;
    std::uint32_t last_serial = 0;
    std::uint32_t days = 0;
    std::uint32_t names = 0;
};

tithi_stats tithi_of(const cg::patro_tables& t) {
    tithi_stats stats;
    std::array<bool, 256> seen{};
    for (std::size_t i = 0; i < t.day_tithi.size(); ++i) {
        const std::uint8_t index = t.day_tithi[i];
        if (index == 0xFFU) {
            continue;
        }
        const auto offset = static_cast<std::uint32_t>(i);
        if (stats.days == 0) {
            stats.first_serial = t.first + offset;
        }
        stats.last_serial = t.first + offset;
        ++stats.days;
        if (!seen.at(index)) {
            seen.at(index) = true;
            ++stats.names;
        }
    }
    return stats;
}

std::uint32_t event_days_of(const cg::patro_tables& t) {
    std::uint32_t days = 0;
    for (std::size_t i = 0; i + 1 < t.day_event_start.size(); ++i) {
        days += t.day_event_start[i] < t.day_event_start[i + 1] ? 1U : 0U;
    }
    return days;
}

std::uint32_t event_names_of(const cg::patro_tables& t) {
    std::vector<std::uint8_t> seen(t.names.size(), 0);
    std::uint32_t names = 0;
    for (const std::uint16_t index : t.events) {
        if (seen[index] == 0) {
            seen[index] = 1;
            ++names;
        }
    }
    return names;
}

} // namespace

TEST_CASE("the patro tables match the hub summary", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    const cg::patro_summary summary = read_summary();
    CHECK(t.first == summary.first_serial);
    CHECK(t.first + t.days - 1U == summary.last_serial);
    CHECK(t.days == summary.days);
    CHECK(date_of_serial(b, t.first) == summary.first_day);
    CHECK(date_of_serial(b, t.first + t.days - 1U) == summary.last_day);

    const tithi_stats tithi = tithi_of(t);
    CHECK(tithi.first_serial == summary.tithi_first_serial);
    CHECK(tithi.last_serial == summary.tithi_first_serial + summary.tithi_days - 1U);
    CHECK(tithi.days == summary.tithi_days);
    CHECK(tithi.names == summary.tithi_names);
    CHECK(date_of_serial(b, tithi.first_serial) == summary.tithi_first_day);
    CHECK(date_of_serial(b, tithi.last_serial) == summary.tithi_last_day);

    CHECK(event_days_of(t) == summary.event_days);
    CHECK(t.events.size() == summary.events);
    CHECK(t.holidays.size() == summary.holidays);
    CHECK(t.names.size() == summary.names);
    CHECK(t.names.size() >= summary.tithi_names);
    CHECK(event_names_of(t) == summary.event_names);
    CHECK(t.name_blob.size() == summary.name_bytes);
    CHECK(t.holiday_bits.size() == (t.days + 63U) / 64U);
    CHECK(t.name_offsets.size() == t.names.size() * 2U + 1U);
    CHECK(t.name_offsets.back() == t.name_blob.size());
    CHECK(cg::validate_patro_tables(t, summary, b.core_tables.month_start).empty());
}

TEST_CASE("the tithi range from the summary is contiguous", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    const cg::patro_summary summary = read_summary();
    REQUIRE(summary.tithi_first_serial >= t.first);
    REQUIRE(summary.tithi_days > 0);
    const std::uint32_t first = summary.tithi_first_serial - t.first;
    const std::uint32_t last = summary.tithi_first_serial + summary.tithi_days - 1U - t.first;
    REQUIRE(last < t.days);
    CHECK(t.day_tithi[first] != 0xFFU);
    CHECK(t.day_tithi[last] != 0xFFU);
    CHECK((first == 0 || t.day_tithi[first - 1] == 0xFFU));
    CHECK((last + 1U == t.days || t.day_tithi[last + 1U] == 0xFFU));
    std::uint32_t tithi_days = 0;
    for (std::uint32_t i = first; i <= last; ++i) {
        CHECK(t.day_tithi[i] < summary.tithi_names);
        ++tithi_days;
    }
    CHECK(tithi_days == summary.tithi_days);
}

TEST_CASE("removing one tithi day fails and names the missing date", "[patro]") {
    const built b = build_all();
    cg::patro_input broken = b.input;
    // Drop the second tithi day.
    std::size_t seen = 0;
    std::uint32_t missing = 0;
    for (std::size_t i = 0; i < broken.days.size(); ++i) {
        if (!broken.days[i].has_tithi) {
            continue;
        }
        ++seen;
        if (seen == 2) {
            missing = serial(b, broken.days[i].year, broken.days[i].month, broken.days[i].day);
            broken.days.erase(broken.days.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
    REQUIRE(missing != 0);
    const auto rebuilt = cg::build_patro_tables(broken, b.core_tables.month_start);
    REQUIRE_FALSE(rebuilt.error.empty());
    CHECK_THAT(rebuilt.error, ContainsSubstring("tithi gap at " + date_of_serial(b, missing)));
}

TEST_CASE("holidays are strictly ascending and each has its bit set", "[patro]") {
    const built b = build_all();
    const cg::patro_tables& t = b.tables;
    const cg::patro_summary summary = read_summary();
    REQUIRE(t.holidays.size() == summary.holidays);
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

TEST_CASE("a summary that disagrees is rejected by field and both values", "[patro]") {
    const built b = build_all();
    cg::patro_summary summary = read_summary();
    ++summary.names;
    const std::string error =
        cg::validate_patro_tables(b.tables, summary, b.core_tables.month_start);
    CHECK_THAT(error, ContainsSubstring("names"));
    CHECK_THAT(error, ContainsSubstring(std::to_string(summary.names)));
    CHECK_THAT(error, ContainsSubstring(std::to_string(b.tables.names.size())));
}

TEST_CASE("a summary without every field is rejected naming the field", "[patro]") {
    const auto parsed = cg::parse_patro_summary(R"({"schema": 1})");
    REQUIRE_FALSE(parsed.error.empty());
    CHECK_THAT(parsed.error, ContainsSubstring("first_year"));
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
