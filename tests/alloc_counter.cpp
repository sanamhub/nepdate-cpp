// Replaces every global operator new and delete with counting versions (ADR-0004 §3). Linked only
// into nepdate_alloc_tests. Never throws, so it works in the noexcept preset too: an exhausted
// heap aborts.

#include "alloc_counter.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables): the counter must outlive every allocation
std::atomic<std::size_t> g_allocations{0};
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

// NOLINTBEGIN(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory): operator new and delete are implemented with malloc and free on purpose
void* allocate(std::size_t size) noexcept {
    g_allocations.fetch_add(1, std::memory_order_relaxed);
    return std::malloc(size == 0 ? 1 : size);
}

void* allocate_aligned(std::size_t size, std::align_val_t align) noexcept {
    g_allocations.fetch_add(1, std::memory_order_relaxed);
    const auto alignment = static_cast<std::size_t>(align);
    const std::size_t rounded =
        size == 0 ? alignment : (size + alignment - 1) / alignment * alignment;
#if defined(_WIN32)
    return _aligned_malloc(rounded, alignment);
#else
    return std::aligned_alloc(alignment, rounded);
#endif
}

void* checked(void* p) noexcept {
    if (p == nullptr) {
        std::abort();
    }
    return p;
}

void release(void* p) noexcept {
    std::free(p);
}

void release_aligned(void* p) noexcept {
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
// NOLINTEND(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)

} // namespace

std::size_t nepdate_test::allocation_count() noexcept {
    return g_allocations.load(std::memory_order_relaxed);
}

// NOLINTBEGIN(misc-new-delete-overloads): the whole global set is replaced on purpose
void* operator new(std::size_t size) {
    return checked(allocate(size));
}
void* operator new[](std::size_t size) {
    return checked(allocate(size));
}
void* operator new(std::size_t size, const std::nothrow_t& /*unused*/) noexcept {
    return allocate(size);
}
void* operator new[](std::size_t size, const std::nothrow_t& /*unused*/) noexcept {
    return allocate(size);
}
void* operator new(std::size_t size, std::align_val_t align) {
    return checked(allocate_aligned(size, align));
}
void* operator new[](std::size_t size, std::align_val_t align) {
    return checked(allocate_aligned(size, align));
}
void* operator new(std::size_t size, std::align_val_t align,
                   const std::nothrow_t& /*unused*/) noexcept {
    return allocate_aligned(size, align);
}
void* operator new[](std::size_t size, std::align_val_t align,
                     const std::nothrow_t& /*unused*/) noexcept {
    return allocate_aligned(size, align);
}

void operator delete(void* p) noexcept {
    release(p);
}
void operator delete[](void* p) noexcept {
    release(p);
}
void operator delete(void* p, std::size_t /*size*/) noexcept {
    release(p);
}
void operator delete[](void* p, std::size_t /*size*/) noexcept {
    release(p);
}
void operator delete(void* p, const std::nothrow_t& /*unused*/) noexcept {
    release(p);
}
void operator delete[](void* p, const std::nothrow_t& /*unused*/) noexcept {
    release(p);
}
void operator delete(void* p, std::align_val_t /*align*/) noexcept {
    release_aligned(p);
}
void operator delete[](void* p, std::align_val_t /*align*/) noexcept {
    release_aligned(p);
}
void operator delete(void* p, std::size_t /*size*/, std::align_val_t /*align*/) noexcept {
    release_aligned(p);
}
void operator delete[](void* p, std::size_t /*size*/, std::align_val_t /*align*/) noexcept {
    release_aligned(p);
}
void operator delete(void* p, std::align_val_t /*align*/,
                     const std::nothrow_t& /*unused*/) noexcept {
    release_aligned(p);
}
void operator delete[](void* p, std::align_val_t /*align*/,
                       const std::nothrow_t& /*unused*/) noexcept {
    release_aligned(p);
}
// NOLINTEND(misc-new-delete-overloads)
