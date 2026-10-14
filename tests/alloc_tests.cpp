// C1-08 and C3-08: the paths of ADR-0002 §6 allocate nothing. Each runs 1000 times between two
// reads of the counter; no Catch2 macro runs inside the measured block.

#include "alloc_counter.hpp"

#include <nepdate/date.hpp>
#include <nepdate/date_range.hpp>
#include <nepdate/fiscal_year.hpp>
#include <nepdate/format.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
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

TEST_CASE("add_months and add_years allocate nothing", "[alloc]") {
    const date d = make_date(2081, 4, 32).value();
    const auto months = allocations_of([d](int i) {
        sink(static_cast<std::size_t>(d.add_months(i % 100 - 50, nepdate::overflow::spill)->day()));
    });
    const auto years = allocations_of(
        [d](int i) { sink(static_cast<std::size_t>(d.add_years(i % 100 - 50)->day())); });
    CHECK(months == 0);
    CHECK(years == 0);
}

TEST_CASE("diff allocates nothing", "[alloc]") {
    const date a = make_date(2081, 4, 32).value();
    const auto n = allocations_of([a](int i) {
        const date b = a.add_days(i * 37 - 18000).value();
        sink(static_cast<std::size_t>(a.diff(b).days));
    });
    CHECK(n == 0);
}

TEST_CASE("date_range construction and a full iteration of of_year(2081) allocate nothing",
          "[alloc]") {
    const date a = make_date(2081, 4, 1).value();
    const auto build = allocations_of([a](int i) {
        const auto r = nepdate::date_range::make(a, a.add_days(i).value());
        const auto month = nepdate::date_range::of_month(2081, nepdate::month::shrawan);
        const auto year = nepdate::date_range::of_year(1901 + i % 299);
        sink(r->size() + month->size() + year->size() + static_cast<std::size_t>(r->contains(a)));
    });
    const auto iterate = allocations_of([](int /*i*/) {
        for (const date d : nepdate::date_range::of_year(2081).value()) {
            sink(static_cast<std::size_t>(d.day()));
        }
    });
    CHECK(build == 0);
    CHECK(iterate == 0);
}

TEST_CASE("fiscal_year members other than label allocate nothing", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto n = allocations_of([d](int i) {
        const auto fy = nepdate::fiscal_year::of(d.add_days(i).value());
        const auto q = nepdate::quarter_of(d);
        std::array<char, 32> buffer{};
        const auto label = fy.to_chars_label(buffer.data(), buffer.data() + buffer.size(),
                                             i % 2 == 0 ? nepdate::lang::en : nepdate::lang::ne);
        sink(static_cast<std::size_t>(fy.start_year() + fy.start()->day() + fy.end()->day()) +
             fy.range()->size() + fy.quarter_range(q)->size() +
             static_cast<std::size_t>(nepdate::months_of(q)[0]) +
             static_cast<std::size_t>(label.ptr - buffer.data()) +
             static_cast<std::size_t>(fy == nepdate::fiscal_year{2081}));
    });
    CHECK(n == 0);
}

TEST_CASE("every to_chars_* writes without allocating", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto n = allocations_of([d](int /*i*/) {
        std::array<char, nepdate::max_long_size> buf{};
        const auto a = nepdate::to_chars(buf.data(), buf.data() + buf.size(), d);
        const auto b = nepdate::to_chars_iso(buf.data(), buf.data() + buf.size(), d);
        const auto c = nepdate::to_chars_short(
            buf.data(), buf.data() + buf.size(), d,
            {nepdate::date_order::dmy, nepdate::separator::dash, false}, nepdate::lang::ne);
        const auto e = nepdate::to_chars_long(buf.data(), buf.data() + buf.size(), d,
                                              {.weekday = true}, nepdate::lang::ne);
        const auto f = nepdate::to_chars_pattern(buf.data(), buf.data() + buf.size(), d,
                                                 "MMMM d, yyyy", nepdate::lang::ne);
        const auto g = nepdate::to_chars_number(buf.data(), buf.data() + buf.size(), 2081, 4,
                                                nepdate::lang::ne);
        sink(static_cast<std::size_t>(a.ptr - buf.data() + b.ptr - buf.data() + c.ptr - buf.data() +
                                      e.ptr - buf.data() + f.ptr - buf.data() + g.ptr -
                                      buf.data()));
    });
    CHECK(n == 0);
}

TEST_CASE("month and weekday names allocate nothing", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto n = allocations_of([d](int i) {
        const nepdate::month m = static_cast<nepdate::month>(1 + i % 12);
        sink(nepdate::month_name(m, nepdate::lang::en).size() +
             nepdate::month_name(m, nepdate::lang::ne).size() +
             nepdate::month_short_name(m, nepdate::lang::en).size() +
             nepdate::month_short_name(m, nepdate::lang::ne).size() +
             nepdate::weekday_name(d.weekday(), nepdate::lang::en).size() +
             nepdate::weekday_short_name(d.weekday(), nepdate::lang::ne).size());
    });
    CHECK(n == 0);
}

TEST_CASE("parse and parse_lenient allocate nothing, success or error", "[alloc]") {
    const auto n = allocations_of([](int /*i*/) {
        const auto a = nepdate::parse("2080/05/15");
        const auto b = nepdate::parse("not a date");
        const auto c = nepdate::parse("२०८०/०५/१५");
        const auto e = nepdate::parse_lenient("15 Shrawan 2080");
        const auto f = nepdate::parse_lenient("Shrawan Bhadra 2080");
        const auto g = nepdate::parse_lenient("");
        sink(static_cast<std::size_t>(a->day()) + static_cast<std::size_t>(b.error()) +
             static_cast<std::size_t>(c->month()) + static_cast<std::size_t>(e->year()) +
             static_cast<std::size_t>(f.error()) + static_cast<std::size_t>(g.error()));
    });
    CHECK(n == 0);
}

TEST_CASE("from_sys_time and today allocate nothing", "[alloc]") {
    const auto n = allocations_of([](int /*i*/) {
        const auto a = nepdate::from_sys_time(
            std::chrono::sys_days{std::chrono::year{2024} / 7 / 30}, nepdate::nepal_utc_offset);
        const auto b = nepdate::today();
        sink(static_cast<std::size_t>(a->day()) + static_cast<std::size_t>(b->day()));
    });
    CHECK(n == 0);
}

TEST_CASE("to_string(d) allocates nothing", "[alloc]") {
    const date d = make_date(2081, 4, 15).value();
    const auto n = allocations_of([d](int /*i*/) { sink(nepdate::to_string(d).size()); });
    // The text fits every small-string buffer; only MSVC's debug iterator proxy can allocate.
    CHECK(n == static_cast<std::size_t>(kRuns) * nepdate_test::kStringProxyAllocations);
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
