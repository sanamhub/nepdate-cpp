#ifndef NEPDATE_BENCH_ALLOC_COUNTER_HPP
#define NEPDATE_BENCH_ALLOC_COUNTER_HPP

#include <cstddef>

namespace nepdate_bench {

// Calls to any global operator new since the program started, and the sum of their requested
// sizes (alloc_counter.cpp). Used by the benchmark::MemoryManager to report allocations per run.
std::size_t allocation_count() noexcept;
std::size_t allocation_bytes() noexcept;

} // namespace nepdate_bench

#endif // NEPDATE_BENCH_ALLOC_COUNTER_HPP
