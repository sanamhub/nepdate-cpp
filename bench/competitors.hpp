#ifndef NEPDATE_BENCH_COMPETITORS_HPP
#define NEPDATE_BENCH_COMPETITORS_HPP

namespace nepdate_bench {

// One competitor of docs/competitors.md, wrapped so the benchmark calls each the same way. All
// dates are 1-based year/month/day. to_ad and to_bs return false when the competitor has no
// answer for the input (out of its published range); they never print.
struct rival {
    const char* name;
    bool (*to_ad)(int y, int m, int d, int& gy, int& gm, int& gd);
    bool (*to_bs)(int y, int m, int d, int& by, int& bm, int& bd);
};

const rival* rivals() noexcept;
int rival_count() noexcept;

} // namespace nepdate_bench

#endif // NEPDATE_BENCH_COMPETITORS_HPP
