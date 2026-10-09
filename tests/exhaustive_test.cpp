// C1-06: all 109,212 serials, BS -> serial -> AD -> BS, and Neri-Schneider against Hinnant.

#include "hinnant_reference.hpp"

#include <nepdate/date.hpp>
#include <nepdate/detail/civil.hpp>
#include <nepdate/detail/serial.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>

using nepdate::date;
using nepdate::detail::from_serial;
using nepdate::detail::kEpochUnixDays;
using nepdate::detail::kMaxSerial;
using nepdate::detail::to_serial;

TEST_CASE("every serial round-trips BS -> serial -> AD -> BS", "[exhaustive]") {
    std::uint32_t failures = 0;
    for (std::uint32_t s = 0; s <= kMaxSerial; ++s) {
        const date bs = from_serial(s);
        const std::chrono::year_month_day ad = bs.to_year_month_day();
        const auto back = nepdate::from_gregorian(ad);
        const bool ok =
            to_serial(bs) == s && back.has_value() && *back == bs &&
            bs.to_sys_days().time_since_epoch().count() == static_cast<int>(s) + kEpochUnixDays;
        failures += ok ? 0U : 1U;
    }
    CHECK(failures == 0);
}

TEST_CASE("Neri-Schneider agrees with Hinnant on every supported day", "[exhaustive]") {
    std::uint32_t failures = 0;
    for (std::uint32_t s = 0; s <= kMaxSerial; ++s) {
        const int days = static_cast<int>(s) + kEpochUnixDays;
        const auto ns = nepdate::detail::civil_from_days(days);
        const auto hh = nepdate_test::civil_from_days(days);
        const bool ok = ns.year == hh.y && static_cast<int>(ns.month) == hh.m &&
                        static_cast<int>(ns.day) == hh.d &&
                        nepdate::detail::days_from_civil(ns.year, ns.month, ns.day) == days &&
                        nepdate_test::days_from_civil(hh.y, hh.m, hh.d) == days;
        failures += ok ? 0U : 1U;
    }
    CHECK(failures == 0);
}
