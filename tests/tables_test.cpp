// The generated tables of ADR-0002 §2.

#include <nepdate/detail/calendar_tables.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

using nepdate::detail::kBlockMonth;
using nepdate::detail::kCalendarFingerprint;
using nepdate::detail::kMaxSerial;
using nepdate::detail::kMonthStart;

TEST_CASE("kMonthStart runs from 0 to 109212 and strictly increases", "[tables]") {
    STATIC_REQUIRE(kMonthStart.size() == 3589);
    STATIC_REQUIRE(kMonthStart[0] == 0);
    STATIC_REQUIRE(kMonthStart[3588] == 109212);
    STATIC_REQUIRE(kMaxSerial == 109211);
    for (std::size_t i = 1; i < kMonthStart.size(); ++i) {
        REQUIRE(kMonthStart[i - 1] < kMonthStart[i]);
    }
}

TEST_CASE("every serial is at most two steps from its block's month", "[tables]") {
    STATIC_REQUIRE(kBlockMonth.size() == 3413);
    for (std::uint32_t s = 0; s <= kMaxSerial; ++s) {
        std::size_t i = kBlockMonth[s >> 5U];
        REQUIRE(kMonthStart[i] <= s);
        int steps = 0;
        while (kMonthStart[i + 1] <= s) {
            ++i;
            ++steps;
        }
        REQUIRE(steps <= 2);
    }
}

TEST_CASE("kCalendarFingerprint is FNV-1a 64 of the month lengths", "[tables]") {
    std::uint64_t hash = 0xcbf29ce484222325;
    for (std::size_t i = 0; i + 1 < kMonthStart.size(); ++i) {
        hash ^= static_cast<std::uint8_t>(kMonthStart[i + 1] - kMonthStart[i]);
        hash *= 0x100000001b3;
    }
    CHECK(hash == kCalendarFingerprint);
}
