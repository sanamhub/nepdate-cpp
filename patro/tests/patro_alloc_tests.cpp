// C4-06: the patro paths of ADR-0002 section 6 allocate nothing. Each runs 1000 times between
// two reads of the counter (the same counter as tests/alloc_counter.cpp); no Catch2 macro runs
// inside the measured block.

#include "alloc_counter.hpp"

#include <nepdate/patro.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>

using nepdate::date;
using nepdate::lang;
using nepdate::make_date;
using nepdate_test::allocation_count;
namespace patro = nepdate::patro;

namespace {

constexpr int kRuns = 1000;

// Keeps the optimiser from dropping the measured calls.
volatile std::size_t g_sink = 0;

void sink(std::size_t value) {
    g_sink = g_sink + value;
}

template <class F> std::size_t allocations_of(F f) {
    const std::size_t before = allocation_count();
    for (int i = 0; i < kRuns; ++i) {
        f(i);
    }
    return allocation_count() - before;
}

} // namespace

TEST_CASE("info allocates nothing, inside and outside the coverage", "[alloc]") {
    const date inside = make_date(2081, 1, 1).value();
    const auto n = allocations_of([inside](int i) {
        const patro::day_info a = patro::info(inside);
        const patro::day_info b = patro::info(date::max());
        sink(static_cast<std::size_t>(a.is_public_holiday()) +
             static_cast<std::size_t>(b.is_public_holiday()) + static_cast<std::size_t>(i % 2));
    });
    CHECK(n == 0);
}

TEST_CASE("day_info accessors allocate nothing", "[alloc]") {
    const date inside = make_date(2081, 1, 1).value();
    const patro::day_info day = patro::info(inside);
    const auto n = allocations_of([day](int /*i*/) {
        const std::size_t events = day.events().size();
        sink(events + static_cast<std::size_t>(day.tithi().has_value()) +
             static_cast<std::size_t>(day.is_public_holiday()));
    });
    CHECK(n == 0);
}

TEST_CASE("tithi and event names allocate nothing, in both languages", "[alloc]") {
    const patro::day_info day = patro::info(make_date(2081, 1, 1).value());
    const auto n = allocations_of([day](int i) {
        std::size_t total = 0;
        if (const auto t = day.tithi()) {
            total += t->name(i % 2 == 0 ? lang::en : lang::ne).size();
        }
        for (const patro::event e : day.events()) {
            total += e.name(i % 2 == 0 ? lang::en : lang::ne).size();
        }
        sink(total);
    });
    CHECK(n == 0);
}

TEST_CASE("iterating events() allocates nothing", "[alloc]") {
    const patro::day_info day = patro::info(make_date(2080, 11, 1).value());
    const auto n = allocations_of([day](int /*i*/) {
        std::size_t total = 0;
        for (const patro::event e : day.events()) {
            total += e.name(lang::en).size();
        }
        sink(total);
    });
    CHECK(n == 0);
}

TEST_CASE("coverage and public_holidays allocate nothing", "[alloc]") {
    const auto n = allocations_of([](int /*i*/) {
        const nepdate::date_range range = patro::coverage();
        const std::span<const date> holidays = patro::public_holidays();
        sink(range.size() + holidays.size());
    });
    CHECK(n == 0);
}

TEST_CASE("the counter sees an allocation", "[alloc]") {
    // Without this the tests above would also pass with a counter that never counts.
    const auto n = allocations_of([](int /*i*/) {
        // Longer than every small-string buffer, so the heap is hit.
        const std::string s(64, 'x'); // NOLINT: measured on purpose
        sink(s.size());
    });
    CHECK(n == static_cast<std::size_t>(kRuns) * (1U + nepdate_test::kStringProxyAllocations));
}
