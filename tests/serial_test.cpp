// C1-04: serial conversion (ADR-0002 §3).

#include <nepdate/date.hpp>
#include <nepdate/detail/calendar_tables.hpp>
#include <nepdate/detail/serial.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using nepdate::date;
using nepdate::make_date;
using nepdate::detail::from_serial;
using nepdate::detail::kMaxSerial;
using nepdate::detail::to_serial;

TEST_CASE("min and max are serials 0 and 109211", "[serial]") {
    STATIC_REQUIRE(to_serial(date::min()) == 0);
    STATIC_REQUIRE(to_serial(date::max()) == 109211);
    STATIC_REQUIRE(from_serial(0) == date::min());
    STATIC_REQUIRE(from_serial(kMaxSerial) == date::max());
    STATIC_REQUIRE(to_serial(make_date(1901, 2, 1).value()) == 31);
}

TEST_CASE("every serial round-trips through from_serial and to_serial", "[serial]") {
    for (std::uint32_t s = 0; s <= kMaxSerial; ++s) {
        REQUIRE(to_serial(from_serial(s)) == s);
    }
}

TEST_CASE("consecutive serials are consecutive days", "[serial]") {
    date previous = from_serial(0);
    for (std::uint32_t s = 1; s <= kMaxSerial; ++s) {
        const date d = from_serial(s);
        REQUIRE(previous < d);
        if (d.day() == 1) {
            REQUIRE(previous.day() == previous.month_length());
        } else {
            REQUIRE(d.day() == previous.day() + 1);
            REQUIRE(d.month() == previous.month());
        }
        previous = d;
    }
}
