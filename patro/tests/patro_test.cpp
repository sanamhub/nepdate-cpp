// The patro queries (PATRO section 2) against the PATRO section 4 anchors. This file is
// UTF-8; the library headers are ASCII.

#include <nepdate/format.hpp>
#include <nepdate/patro.hpp>

#include "patro_summary.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// NOLINTBEGIN(bugprone-unchecked-optional-access): every std::optional is dereferenced
// after a REQUIRE(has_value()) guard, which the purely syntactic check cannot see.

using nepdate::date;
using nepdate::lang;
using nepdate::make_date;
namespace patro = nepdate::patro;

namespace {

// The hub summary every port vendors; the generated tables and the public queries must agree
// with it.
nepdate::test::patro_summary summary() {
    const auto parsed = nepdate::test::read_patro_summary(std::string(NEPDATE_SHARED_DIR) +
                                                          "/spec/vectors/patro-summary.json");
    REQUIRE(parsed.has_value());
    return *parsed;
}

date d(int y, int m, int day) {
    return make_date(y, m, day).value();
}

// The English event names of a day, in order.
std::vector<std::string_view> events_en(date dt) {
    std::vector<std::string_view> names;
    const patro::day_info day = patro::info(dt);
    for (const patro::event e : day.events()) {
        names.push_back(e.name(lang::en));
    }
    return names;
}

} // namespace

TEST_CASE("2081-01-01: Panchami, holiday, New Year first", "[patro]") {
    const patro::day_info day = patro::info(d(2081, 1, 1));
    REQUIRE(day.tithi().has_value());
    CHECK(day.tithi()->name(lang::ne) == "पञ्चमी");
    CHECK(day.tithi()->name(lang::en) == "Panchami");
    CHECK(day.is_public_holiday());
    const std::span<const patro::event> events = day.events();
    REQUIRE(events.size() == 3);
    CHECK(events[0].name(lang::ne) == "नयाँ वर्ष");
    CHECK(events[0].name(lang::en) == "New Year");
    CHECK(events[1].name(lang::en) == "Mesh Sankranti");
    CHECK(events[1].name(lang::ne) == "मेष संक्रान्ति");
    CHECK(events[2].name(lang::en) == "Bisket Jatra");
}

TEST_CASE("2081-01-02: Shashthi, no holiday, no events", "[patro]") {
    const patro::day_info day = patro::info(d(2081, 1, 2));
    REQUIRE(day.tithi().has_value());
    CHECK(day.tithi()->name(lang::en) == "Shashthi");
    CHECK(day.tithi()->name(lang::ne) == "षष्ठी");
    CHECK_FALSE(day.is_public_holiday());
    CHECK(day.events().empty());
}

TEST_CASE("2081-02-15: Panchami, Republic Day", "[patro]") {
    const patro::day_info day = patro::info(d(2081, 2, 15));
    REQUIRE(day.tithi().has_value());
    CHECK(day.tithi()->name(lang::en) == "Panchami");
    CHECK(day.is_public_holiday());
    const std::span<const patro::event> events = day.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].name(lang::en) == "Republic Day");
}

TEST_CASE("2080-11-01: Chaturthi and its five events in order", "[patro]") {
    const patro::day_info day = patro::info(d(2080, 11, 1));
    REQUIRE(day.tithi().has_value());
    CHECK(day.tithi()->name(lang::en) == "Chaturthi");
    CHECK_FALSE(day.is_public_holiday());
    CHECK(events_en(d(2080, 11, 1)) ==
          std::vector<std::string_view>{"Mangala Chauthi Brata", "People's War Day",
                                        "Tilakund Chauthi", "Kumbha Sankranti", "World Radio Day"});
}

TEST_CASE("2081-04-15: Dashami and its three events in order", "[patro]") {
    const patro::day_info day = patro::info(d(2081, 4, 15));
    REQUIRE(day.tithi().has_value());
    CHECK(day.tithi()->name(lang::en) == "Dashami");
    CHECK_FALSE(day.is_public_holiday());
    CHECK(events_en(d(2081, 4, 15)) ==
          std::vector<std::string_view>{"Kheer Khane Din", "World Day Against Human Trafficking",
                                        "International Friendship Day"});
}

TEST_CASE("dates without data give an empty day_info", "[patro]") {
    for (const date dt : {d(2070, 6, 15), d(2090, 1, 1), date::min(), date::max()}) {
        INFO(nepdate::to_iso_string(dt));
        const patro::day_info day = patro::info(dt);
        CHECK_FALSE(day.tithi().has_value());
        CHECK_FALSE(day.is_public_holiday());
        CHECK(day.events().empty());
    }
}

TEST_CASE("coverage matches the hub summary", "[patro]") {
    const nepdate::test::patro_summary expected = summary();
    const nepdate::date_range range = patro::coverage();
    CHECK(nepdate::to_iso_string(range.first()) == expected.first_day);
    CHECK(nepdate::to_iso_string(range.last()) == expected.last_day);
    CHECK(range.size() == expected.days);
}

TEST_CASE("public_holidays matches the hub summary, ascending, each a holiday", "[patro]") {
    const nepdate::test::patro_summary expected = summary();
    const std::span<const date> holidays = patro::public_holidays();
    REQUIRE(holidays.size() == expected.holidays);
    for (std::size_t i = 0; i < holidays.size(); ++i) {
        CAPTURE(i);
        if (i != 0) {
            CHECK(holidays[i - 1] < holidays[i]);
        }
        CHECK(patro::info(holidays[i]).is_public_holiday());
    }
}

TEST_CASE("the patro types have the ADR-0002 section 7 layout", "[patro]") {
    STATIC_REQUIRE(sizeof(patro::tithi) == 1);
    STATIC_REQUIRE(sizeof(patro::event) == 2);
    STATIC_REQUIRE(sizeof(patro::day_info) == 16);
    STATIC_REQUIRE(std::is_trivially_copyable_v<patro::day_info>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<patro::tithi>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<patro::event>);
}
// NOLINTEND(bugprone-unchecked-optional-access)
