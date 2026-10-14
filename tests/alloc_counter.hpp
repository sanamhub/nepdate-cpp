#ifndef NEPDATE_TESTS_ALLOC_COUNTER_HPP
#define NEPDATE_TESTS_ALLOC_COUNTER_HPP

#include <cstddef>
#include <string> // IWYU pragma: keep (defines _MSVC_STL_VERSION and _ITERATOR_DEBUG_LEVEL on MSVC)

namespace nepdate_test {

// Number of calls to any global operator new since the program started (alloc_counter.cpp).
std::size_t allocation_count() noexcept;

// Heap allocations a std::string makes besides its character buffer. MSVC's debug library
// (_ITERATOR_DEBUG_LEVEL above 0) gives every container a heap-allocated iterator proxy; other
// libraries, and MSVC release builds, allocate none.
#if defined(_MSVC_STL_VERSION) && _ITERATOR_DEBUG_LEVEL > 0
inline constexpr std::size_t kStringProxyAllocations = 1;
#else
inline constexpr std::size_t kStringProxyAllocations = 0;
#endif

} // namespace nepdate_test

#endif // NEPDATE_TESTS_ALLOC_COUNTER_HPP
