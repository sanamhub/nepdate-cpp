#ifndef NEPDATE_TESTS_ALLOC_COUNTER_HPP
#define NEPDATE_TESTS_ALLOC_COUNTER_HPP

#include <cstddef>

namespace nepdate_test {

// Number of calls to any global operator new since the program started (alloc_counter.cpp).
std::size_t allocation_count() noexcept;

} // namespace nepdate_test

#endif // NEPDATE_TESTS_ALLOC_COUNTER_HPP
