// C2-05: date_range, its iterator and the range concepts.

#include <nepdate/date_range.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

using nepdate::date;
using nepdate::date_range;
using nepdate::errc;
using nepdate::make_date;
using nepdate::month;

static_assert(std::random_access_iterator<date_range::iterator>);
static_assert(std::sized_sentinel_for<date_range::iterator, date_range::iterator>);
static_assert(std::is_same_v<std::iter_value_t<date_range::iterator>, date>);
static_assert(std::ranges::random_access_range<date_range>);
static_assert(std::ranges::sized_range<date_range>);
static_assert(std::ranges::common_range<date_range>);
static_assert(std::ranges::view<date_range>);
static_assert(std::ranges::borrowed_range<date_range>);
static_assert(std::is_trivially_copyable_v<date_range>);

namespace {

constexpr date bs(int y, int m, int d) {
    return make_date(y, m, d).value();
}

} // namespace

TEST_CASE("of_year: size is the sum of the month lengths, 364..367", "[range]") {
    for (int y = 1901; y <= 2199; ++y) {
        INFO(y);
        const auto r = date_range::of_year(y);
        REQUIRE(r.has_value());
        std::size_t sum = 0;
        for (int m = 1; m <= 12; ++m) {
            sum += static_cast<std::size_t>(make_date(y, m, 1)->month_length());
        }
        CHECK(r->size() == sum);
        CHECK(r->size() >= 364);
        CHECK(r->size() <= 367);
        CHECK(r->first() == bs(y, 1, 1));
        CHECK(r->last() == make_date(y, 12, 1)->last_of_month());
    }
}

TEST_CASE("of_month: Shrawan 2081 has 32 dates", "[range]") {
    const auto r = date_range::of_month(2081, month::shrawan);
    REQUIRE(r.has_value());
    CHECK(r->size() == 32);
    CHECK(r->first() == bs(2081, 4, 1));
    CHECK(r->last() == bs(2081, 4, 32));

    int visited = 0;
    for (const date d : *r) {
        ++visited;
        if (d == bs(2081, 4, 3)) {
            break;
        }
    }
    CHECK(visited == 3);
}

TEST_CASE("make, of_month and of_year reject what they can't build", "[range]") {
    STATIC_REQUIRE(!date_range::make(date::max(), date::min()).has_value());
    STATIC_REQUIRE(date_range::make(date::min(), date::min())->size() == 1);
    STATIC_REQUIRE(date_range::make(date::min(), date::max())->size() == 109212);
    CHECK(date_range::of_year(2200).error() == errc::out_of_range);
    CHECK(date_range::of_year(1900).error() == errc::out_of_range);
    CHECK(date_range::of_month(2200, month::baishakh).error() == errc::out_of_range);
    CHECK(date_range::of_month(2081, static_cast<month>(13)).error() == errc::invalid_month);
}

TEST_CASE("contains is inclusive at both ends", "[range]") {
    const date_range r = date_range::make(bs(2080, 1, 5), bs(2080, 1, 20)).value();
    CHECK(r.contains(bs(2080, 1, 5)));
    CHECK(r.contains(bs(2080, 1, 20)));
    CHECK(!r.contains(bs(2080, 1, 4)));
    CHECK(!r.contains(bs(2080, 1, 21)));
}

TEST_CASE("the iterator walks of_year(2081) like add_days", "[range]") {
    const date_range r = date_range::of_year(2081).value();
    CHECK(std::ranges::distance(r) == static_cast<std::ptrdiff_t>(r.size()));
    const auto first = r.begin();
    for (std::ptrdiff_t k = 0; k < static_cast<std::ptrdiff_t>(r.size()); ++k) {
        CHECK(first[k] == r.first().add_days(static_cast<int>(k)));
    }
    auto it = r.end();
    --it;
    CHECK(*it == r.last());
    CHECK(it - first == static_cast<std::ptrdiff_t>(r.size()) - 1);
    CHECK(first + 10 == 10 + first);
    CHECK(*(first + 40 - 8) == r.first().add_days(32));
    CHECK(first < it);
    CHECK(r.front() == r.first());
    CHECK(r.back() == r.last());
    CHECK(r[5] == bs(2081, 1, 6));
}

TEST_CASE("a range that ends on max() can still be iterated to its end", "[range]") {
    const date_range r = date_range::make(bs(2199, 12, 28), date::max()).value();
    std::size_t n = 0;
    for (const date d : r) {
        CHECK(r.contains(d));
        ++n;
    }
    CHECK(n == 3);
}

TEST_CASE("ranges compare by their ends", "[range]") {
    const auto a = date_range::make(bs(2080, 1, 1), bs(2080, 1, 15));
    CHECK(a == date_range::make(bs(2080, 1, 1), bs(2080, 1, 15)));
    CHECK(a != date_range::make(bs(2080, 1, 1), bs(2080, 1, 16)));
    CHECK(a != date_range::make(bs(2080, 1, 2), bs(2080, 1, 15)));
}

TEST_CASE("date_range works in a constant expression", "[range][constexpr]") {
    STATIC_REQUIRE(date_range::of_month(2081, month::shrawan)->size() == 32);
    STATIC_REQUIRE(*(date_range::of_year(2081)->begin() + 100) ==
                   bs(2081, 1, 1).add_days(100).value());
    STATIC_REQUIRE(date_range::of_year(2081)->contains(bs(2081, 12, 30)));
}
