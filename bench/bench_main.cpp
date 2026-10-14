// C5-02: B1 to B8 of BENCHMARK section 3 for this library, the ADR-0002 section 8 variant of B1,
// and B1/B2 for each competitor of docs/competitors.md, all over the 64 dates of BENCHMARK
// section 2 (the patro inputs of section 3 for B7/B8). Each operation consumes every part of its
// result, and the counting operator new (alloc_counter.cpp) feeds a benchmark::MemoryManager so
// the report carries bytes per operation.
//
// The "agrees" check prints, before timing, how many of the 64 dates each competitor converts to
// the same date as this library.

#include <nepdate/nepdate.hpp>
#include <nepdate/patro.hpp>

#include <nepdate/detail/calendar_tables.hpp>
#include <nepdate/detail/civil.hpp>
#include <nepdate/detail/serial.hpp>

#include <benchmark/benchmark.h>

#include "alloc_counter.hpp"
#include "competitors.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace {

namespace detail = nepdate::detail;
using nepdate::date;

constexpr std::size_t kInputs = 64;

struct ymd {
    int year;
    int month;
    int day;
};

// BENCHMARK section 2: date i is the BS date at serial s0 + i * ((s1 - s0) / 63), s0 = BS
// 2000-01-01, s1 = BS 2090-12-30.
date bs_input(std::size_t i) {
    const std::uint32_t s0 = detail::to_serial(nepdate::make_date(2000, 1, 1).value());
    const std::uint32_t s1 = detail::to_serial(nepdate::make_date(2090, 12, 30).value());
    return detail::from_serial(s0 + static_cast<std::uint32_t>(i) * ((s1 - s0) / 63U));
}

std::array<ymd, kInputs> bs_fields() {
    std::array<ymd, kInputs> out{};
    for (std::size_t i = 0; i < kInputs; ++i) {
        const date d = bs_input(i);
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

// BENCHMARK section 2: the text inputs for parsing are the 64 dates as "YYYY/MM/DD".
std::array<std::string, kInputs> text_inputs() {
    std::array<std::string, kInputs> out{};
    const auto fields = bs_fields();
    for (std::size_t i = 0; i < kInputs; ++i) {
        std::array<char, 11> buf{};
        std::snprintf(buf.data(), buf.size(), "%04d/%02d/%02d", fields.at(i).year,
                      fields.at(i).month, fields.at(i).day);
        out.at(i) = buf.data();
    }
    return out;
}

// BENCHMARK section 3: date i is the date at serial c0 + i * ((c1 - c0) / 63) over the patro
// coverage c0..c1 (PATRO section 2). Serial days, because date has no default constructor.
std::array<std::uint32_t, kInputs> patro_serials() {
    std::array<std::uint32_t, kInputs> out{};
    const nepdate::date_range coverage = nepdate::patro::coverage();
    const std::uint32_t c0 = detail::to_serial(coverage.first());
    const std::uint32_t c1 = detail::to_serial(coverage.last());
    for (std::size_t i = 0; i < kInputs; ++i) {
        out.at(i) = c0 + static_cast<std::uint32_t>(i) * ((c1 - c0) / 63U);
    }
    return out;
}

void consume(int year, unsigned month, unsigned day) {
    benchmark::DoNotOptimize(year);
    benchmark::DoNotOptimize(month);
    benchmark::DoNotOptimize(day);
}

void consume(std::string_view s) {
    benchmark::DoNotOptimize(s.data());
    benchmark::DoNotOptimize(s.size());
}

// Consumes every part of a day_info: the tithi name, the holiday flag and every event name.
void consume_day_info(const nepdate::patro::day_info& info) {
    if (const auto t = info.tithi()) {
        consume(t->name(nepdate::lang::en));
    }
    benchmark::DoNotOptimize(info.is_public_holiday());
    for (const nepdate::patro::event e : info.events()) {
        consume(e.name(nepdate::lang::en));
    }
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

// B3: add 1000 days.
void b3_add_1000_days(benchmark::State& state) {
    const auto inputs = bs_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const date d = nepdate::make_date(in.year, in.month, in.day).value();
            const date r = d.add_days(1000).value();
            consume(r.year(), static_cast<unsigned>(r.month()), static_cast<unsigned>(r.day()));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b3_add_1000_days);

// B4: add 13 months, clamp.
void b4_add_13_months(benchmark::State& state) {
    const auto inputs = bs_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const date d = nepdate::make_date(in.year, in.month, in.day).value();
            const date r = d.add_months(13, nepdate::overflow::clamp).value();
            consume(r.year(), static_cast<unsigned>(r.month()), static_cast<unsigned>(r.day()));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b4_add_13_months);

// B5: long format, Nepali, with weekday, into the caller's buffer (the C++ long equivalent; the
// std::string form would allocate, this one does not).
void b5_long_nepali_weekday(benchmark::State& state) {
    const auto inputs = bs_fields();
    constexpr nepdate::long_format f{true, true, true};
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            const date d = nepdate::make_date(in.year, in.month, in.day).value();
            std::array<char, nepdate::max_long_size> buf{};
            const auto r = nepdate::to_chars_long(buf.data(), buf.data() + buf.size(), d, f,
                                                  nepdate::lang::ne);
            consume(std::string_view(buf.data(), static_cast<std::size_t>(r.ptr - buf.data())));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b5_long_nepali_weekday);

// B6: strict parse of "YYYY/MM/DD".
void b6_strict_parse(benchmark::State& state) {
    const auto inputs = text_inputs();
    for (auto _ : state) {
        for (const std::string& s : inputs) {
            const auto r = nepdate::parse(s);
            consume(r->year(), static_cast<unsigned>(r->month()), static_cast<unsigned>(r->day()));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b6_strict_parse);

// B7: patro info for the date (tithi, holiday flag, events).
void b7_patro_info(benchmark::State& state) {
    const auto inputs = patro_serials();
    for (auto _ : state) {
        for (const std::uint32_t s : inputs) {
            consume_day_info(nepdate::patro::info(detail::from_serial(s)));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b7_patro_info);

// B8: a month grid: patro info for every day of the month the date is in.
void b8_patro_month_grid(benchmark::State& state) {
    const auto inputs = patro_serials();
    for (auto _ : state) {
        for (const std::uint32_t s : inputs) {
            const date d = detail::from_serial(s);
            const nepdate::date_range month =
                nepdate::date_range::of_month(d.year(), d.month()).value();
            for (const date day : month) {
                consume_day_info(nepdate::patro::info(day));
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}
BENCHMARK(b8_patro_month_grid);

// ---- ADR-0002 section 8 variant of B1, bench only: the AD date of each BS month start is stored
// (as C# does), so BS to AD adds d - 1 days to it instead of running civil_from_days. 3588 entries
// of 4 bytes, about 14 KB more than the library's tables. Kept only if it beats B1 by 10 % on
// both compilers (ADR-0002 section 8); the library still uses Neri-Schneider otherwise.

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
    const date d = nepdate::make_date(in.year, in.month, in.day).value();
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

// ---- Competitors: B1 and B2 only (docs/competitors.md); the other columns are n/a. ----

void rival_to_ad(benchmark::State& state, const nepdate_bench::rival* r) {
    const auto inputs = bs_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            int gy = 0;
            int gm = 0;
            int gd = 0;
            bool ok = r->to_ad(in.year, in.month, in.day, gy, gm, gd);
            consume(gy, static_cast<unsigned>(gm), static_cast<unsigned>(gd));
            benchmark::DoNotOptimize(ok);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}

void rival_to_bs(benchmark::State& state, const nepdate_bench::rival* r) {
    const auto inputs = ad_fields();
    for (auto _ : state) {
        for (const ymd& in : inputs) {
            int by = 0;
            int bm = 0;
            int bd = 0;
            bool ok = r->to_bs(in.year, in.month, in.day, by, bm, bd);
            consume(by, static_cast<unsigned>(bm), static_cast<unsigned>(bd));
            benchmark::DoNotOptimize(ok);
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(kInputs));
}

BENCHMARK_CAPTURE(rival_to_ad, nepdate_cli, &nepdate_bench::rivals()[0]);
BENCHMARK_CAPTURE(rival_to_ad, aatishnn, &nepdate_bench::rivals()[1]);
BENCHMARK_CAPTURE(rival_to_ad, nepalical, &nepdate_bench::rivals()[2]);
BENCHMARK_CAPTURE(rival_to_bs, nepdate_cli, &nepdate_bench::rivals()[0]);
BENCHMARK_CAPTURE(rival_to_bs, aatishnn, &nepdate_bench::rivals()[1]);
BENCHMARK_CAPTURE(rival_to_bs, nepalical, &nepdate_bench::rivals()[2]);

// ---- "agrees" check (BENCHMARK section 4): printed before timing. ----

void print_agrees() {
    const auto bs = bs_fields();
    const auto ad = ad_fields();
    for (int i = 0; i < nepdate_bench::rival_count(); ++i) {
        const nepdate_bench::rival* r = &nepdate_bench::rivals()[i];
        int agree_b1 = 0;
        int no_b1 = 0;
        int agree_b2 = 0;
        int no_b2 = 0;
        for (std::size_t j = 0; j < kInputs; ++j) {
            int gy = 0;
            int gm = 0;
            int gd = 0;
            if (!r->to_ad(bs.at(j).year, bs.at(j).month, bs.at(j).day, gy, gm, gd)) {
                ++no_b1;
            } else if (gy == ad.at(j).year && gm == ad.at(j).month && gd == ad.at(j).day) {
                ++agree_b1;
            }
            int by = 0;
            int bm = 0;
            int bd = 0;
            if (!r->to_bs(ad.at(j).year, ad.at(j).month, ad.at(j).day, by, bm, bd)) {
                ++no_b2;
            } else if (by == bs.at(j).year && bm == bs.at(j).month && bd == bs.at(j).day) {
                ++agree_b2;
            }
        }
        std::printf("%s: B1 agrees %d/64 (no answer %d), B2 agrees %d/64 (no answer %d)\n", r->name,
                    agree_b1, no_b1, agree_b2, no_b2);
    }
}

// Reports allocations per run from the counting operator new (BENCHMARK section 4).
class CountingMemoryManager : public benchmark::MemoryManager {
  public:
    void Start() override {
        start_count_ = nepdate_bench::allocation_count();
        start_bytes_ = nepdate_bench::allocation_bytes();
    }
    void Stop(Result& result) override {
        result.num_allocs =
            static_cast<std::int64_t>(nepdate_bench::allocation_count() - start_count_);
        result.total_allocated_bytes =
            static_cast<std::int64_t>(nepdate_bench::allocation_bytes() - start_bytes_);
    }

  private:
    std::size_t start_count_ = 0;
    std::size_t start_bytes_ = 0;
};

} // namespace

int main(int argc, char** argv) {
    CountingMemoryManager manager;
    benchmark::RegisterMemoryManager(&manager);
    benchmark::Initialize(&argc, argv);
    print_agrees();
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
