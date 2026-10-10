// C1-09: B1 and B2 of BENCHMARK §3 for this library, plus the ADR-0002 §8 variant of B1 that
// stores the AD date of each BS month start. C5-02 adds B3 to B8 and the competitors.

#include <nepdate/date.hpp>
#include <nepdate/detail/calendar_tables.hpp>
#include <nepdate/detail/civil.hpp>
#include <nepdate/detail/serial.hpp>

#include <benchmark/benchmark.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace {

namespace detail = nepdate::detail;

constexpr std::size_t kInputs = 64;

struct ymd {
    int year;
    int month;
    int day;
};

// BENCHMARK §2: date i is the BS date at serial s0 + i * ((s1 - s0) / 63), s0 = BS 2000-01-01,
// s1 = BS 2090-12-30.
nepdate::date bs_input(std::size_t i) {
    const std::uint32_t s0 = detail::to_serial(nepdate::make_date(2000, 1, 1).value());
    const std::uint32_t s1 = detail::to_serial(nepdate::make_date(2090, 12, 30).value());
    return detail::from_serial(s0 + static_cast<std::uint32_t>(i) * ((s1 - s0) / 63U));
}

std::array<ymd, kInputs> bs_fields() {
    std::array<ymd, kInputs> out{};
    for (std::size_t i = 0; i < kInputs; ++i) {
        const nepdate::date d = bs_input(i);
        out.at(i) = ymd{d.year(), static_cast<int>(d.month()), d.day()};
    }
    return out;
}

std::array<ymd, kInputs> ad_fields() {
    std::array<ymd, kInputs> out{};
    for (std::size_t i = 0; i < kInputs; ++i) {
        const auto ad = bs_input(i).to_year_month_day();
        out.at(i) =
            ymd{static_cast<int>(ad.year()), static_cast<int>(static_cast<unsigned>(ad.month())),
                static_cast<int>(static_cast<unsigned>(ad.day()))};
    }
    return out;
}

void consume(int year, unsigned month, unsigned day) {
    benchmark::DoNotOptimize(year);
    benchmark::DoNotOptimize(month);
    benchmark::DoNotOptimize(day);
}

// B1: build the date from (y, m, d) and get the AD (y, m, d).
void b1_bs_to_ad(benchmark::State& state) {
    const auto inputs = bs_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const auto ad = nepdate::make_date(in.year, in.month, in.day)->to_year_month_day();
            consume(static_cast<int>(ad.year()), static_cast<unsigned>(ad.month()),
                    static_cast<unsigned>(ad.day()));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b1_bs_to_ad);

// B2: from AD (y, m, d) to the BS date.
void b2_ad_to_bs(benchmark::State& state) {
    const auto inputs = ad_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const auto bs = nepdate::from_gregorian(in.year, in.month, in.day);
            consume(bs->year(), static_cast<unsigned>(bs->month()),
                    static_cast<unsigned>(bs->day()));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b2_ad_to_bs);

// ADR-0002 §8 variant of B1, bench only: the AD date of each BS month start is stored (as C# does),
// so BS to AD adds d - 1 days to it with at most two month rollovers instead of running
// civil_from_days. 3588 entries of 4 bytes, about 14 KB more than the library's tables.
struct ad_start {
    std::uint16_t year;
    std::uint8_t month;
    std::uint8_t day;
};

std::array<ad_start, detail::kMonthStart.size() - 1> make_ad_starts() {
    std::array<ad_start, detail::kMonthStart.size() - 1> out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        const auto c = detail::civil_from_days(static_cast<int>(detail::kMonthStart.at(i)) +
                                               detail::kEpochUnixDays);
        out.at(i) = ad_start{static_cast<std::uint16_t>(c.year), static_cast<std::uint8_t>(c.month),
                             static_cast<std::uint8_t>(c.day)};
    }
    return out;
}

const auto g_ad_starts = make_ad_starts();

ymd variant_to_ad(const ymd& in) {
    const nepdate::date d = nepdate::make_date(in.year, in.month, in.day).value();
    const ad_start start = g_ad_starts[detail::month_index(d)];
    int year = start.year;
    unsigned month = start.month;
    unsigned day = start.day + static_cast<unsigned>(d.day()) - 1U;
    for (unsigned length = detail::gregorian_month_length(year, month); day > length;
         length = detail::gregorian_month_length(year, month)) {
        day -= length;
        month = month == 12U ? 1U : month + 1U;
        year += month == 1U ? 1 : 0;
    }
    return ymd{year, static_cast<int>(month), static_cast<int>(day)};
}

void b1_variant_stored_ad_start(benchmark::State& state) {
    const auto inputs = bs_fields();
    const auto expected = ad_fields();
    for (std::size_t i = 0; i < kInputs; ++i) {
        const ymd got = variant_to_ad(inputs.at(i));
        const ymd want = expected.at(i);
        if (got.year != want.year || got.month != want.month || got.day != want.day) {
            state.SkipWithError("the variant disagrees with to_year_month_day");
            return;
        }
    }
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const ymd ad = variant_to_ad(in);
            consume(ad.year, static_cast<unsigned>(ad.month), static_cast<unsigned>(ad.day));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b1_variant_stored_ad_start);

} // namespace
