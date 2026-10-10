// C1-08: the paths of ADR-0002 §6 that exist after C1 allocate nothing. Each runs 1000 times
// between two reads of the counter; no Catch2 macro runs inside the measured block.

#include "alloc_counter.hpp"

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <functional>

using nepdate::date;
using nepdate::make_date;
using nepdate_test::allocation_count;

namespace {

constexpr int kRuns = 1000;

// Keeps the optimiser from dropping the measured calls.
volatile std::size_t g_sink = 0;
int* volatile g_escape = nullptr;

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

TEST_CASE("make_date allocates nothing", "[alloc]") {
    const auto n = allocations_of([](int i) {
        sink(static_cast<std::size_t>(make_date(2081, 1 + i % 12, 1 + i % 28).has_value()));
    });
    CHECK(n == 0);
}

TEST_CASE("from_gregorian allocates nothing", "[alloc]") {
    const auto n = allocations_of([](int i) {
        sink(
            static_cast<std::size_t>(nepdate::from_gregorian(2024, 1 + i % 12, 1 + i % 28)->day()));
    });
    CHECK(n == 0);
}

TEST_CASE("to_year_month_day, weekday and day_of_year allocate nothing", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto ymd = allocations_of(
        [d](int /*i*/) { sink(static_cast<unsigned>(d.to_year_month_day().day())); });
    const auto weekday = allocations_of([d](int /*i*/) { sink(d.weekday().c_encoding()); });
    const auto day_of_year =
        allocations_of([d](int /*i*/) { sink(static_cast<std::size_t>(d.day_of_year())); });
    CHECK(ymd == 0);
    CHECK(weekday == 0);
    CHECK(day_of_year == 0);
}

TEST_CASE("comparison and std::hash allocate nothing", "[alloc]") {
    const date a = make_date(2081, 4, 15).value();
    const date b = make_date(2081, 4, 16).value();
    const auto compare = allocations_of([a, b](int /*i*/) {
        sink(static_cast<std::size_t>(a < b) + static_cast<std::size_t>(a == b));
    });
    const auto hash = allocations_of([a](int /*i*/) { sink(std::hash<date>{}(a)); });
    CHECK(compare == 0);
    CHECK(hash == 0);
}

TEST_CASE("add_days, next, prev and days_until allocate nothing", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto add = allocations_of(
        [d](int i) { sink(static_cast<std::size_t>(d.add_days(i - kRuns / 2)->day())); });
    const auto next =
        allocations_of([d](int /*i*/) { sink(static_cast<std::size_t>(d.next()->day())); });
    const auto prev =
        allocations_of([d](int /*i*/) { sink(static_cast<std::size_t>(d.prev()->day())); });
    const auto until = allocations_of(
        [d](int /*i*/) { sink(static_cast<std::size_t>(d.days_until(date::max()))); });
    CHECK(add == 0);
    CHECK(next == 0);
    CHECK(prev == 0);
    CHECK(until == 0);
}

TEST_CASE("the counter sees an allocation", "[alloc]") {
    // Without this the tests above would also pass with a counter that never counts.
    const auto n = allocations_of([](int /*i*/) {
        // The pointer escapes through a volatile, so the compiler can't elide the new/delete pair.
        g_escape = new int(7); // NOLINT(cppcoreguidelines-owning-memory): measured on purpose
        sink(static_cast<std::size_t>(*g_escape));
        delete g_escape; // NOLINT(cppcoreguidelines-owning-memory): measured on purpose
    });
    CHECK(n == static_cast<std::size_t>(kRuns));
}
