// C4-03: the patro queries (PATRO section 2) against the PATRO section 4 anchors. This file is
// UTF-8; the library headers are ASCII.

#include <nepdate/format.hpp>
#include <nepdate/patro.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

using nepdate::date;
using nepdate::lang;
using nepdate::make_date;
namespace patro = nepdate::patro;

namespace {

date d(int y, int m, int day) {
    return make_date(y, m, day).value();
}

// The English event names of a day, in order.
std::vector<std::string_view> events_en(date dt) {
    std::vector<std::string_view> names;
    for (const patro::event e : patro::info(dt).events()) {
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

TEST_CASE("coverage is 2077-01-01..2084-08-20", "[patro]") {
    const nepdate::date_range range = patro::coverage();
    CHECK(range.first() == d(2077, 1, 1));
    CHECK(range.last() == d(2084, 8, 20));
    CHECK(range.size() == 2793);
}

TEST_CASE("public_holidays has 294 ascending dates, each a holiday", "[patro]") {
    const std::span<const date> holidays = patro::public_holidays();
    REQUIRE(holidays.size() == 294);
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
