// The ThreadSanitizer test of ADR-0004 section 3. Eight threads read info and do date
// arithmetic over every covered day; all data is immutable, so TSan must stay clean. Runs on
// every platform too, just without the race detector.
#include <nepdate/patro.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <thread>

using nepdate::date;
using nepdate::lang;
namespace patro = nepdate::patro;

TEST_CASE("info and add_days from eight threads over the coverage", "[patro][tsan]") {
    const nepdate::date_range range = patro::coverage();
    constexpr std::size_t kThreads = 8;
    std::array<std::size_t, kThreads> totals{};
    std::array<std::thread, kThreads> threads;
    for (std::size_t t = 0; t < kThreads; ++t) {
        threads[t] = std::thread([&range, &totals, t] {
            std::size_t total = 0;
            for (std::size_t i = t; i < range.size(); i += kThreads) {
                const date d = range.first().add_days(static_cast<int>(i)).value();
                const patro::day_info info = patro::info(d);
                total += static_cast<std::size_t>(info.is_public_holiday());
                if (const auto tithi = info.tithi()) {
                    total += tithi->name(lang::en).size();
                }
                for (const patro::event e : info.events()) {
                    total += e.name(lang::en).size();
                }
                total += static_cast<std::size_t>(d.add_days(3)->day());
            }
            totals[t] = total;
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    std::size_t sum = 0;
    for (const std::size_t total : totals) {
        sum += total;
    }
    CHECK(sum > 0);
}
