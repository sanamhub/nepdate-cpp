// Replaces every global operator new and delete with counting versions for the benchmark
// (BENCHMARK section 4, ADR-0004 section 5). Only calls are counted, not frees, so the
// benchmark::MemoryManager reports bytes allocated per operation. Never throws.

#include "alloc_counter.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace {

std::atomic<std::size_t> g_count{0};
std::atomic<std::size_t> g_bytes{0};

void* allocate(std::size_t size) noexcept {
    g_count.fetch_add(1, std::memory_order_relaxed);
    g_bytes.fetch_add(size, std::memory_order_relaxed);
    return std::malloc(size == 0 ? 1 : size); // NOLINT(cppcoreguidelines-no-malloc): this is new
}

void* allocate_aligned(std::size_t size, std::align_val_t align) noexcept {
    g_count.fetch_add(1, std::memory_order_relaxed);
    g_bytes.fetch_add(size, std::memory_order_relaxed);
    const auto alignment = static_cast<std::size_t>(align);
    const std::size_t rounded =
        size == 0 ? alignment : (size + alignment - 1) / alignment * alignment;
#if defined(_WIN32)
    return _aligned_malloc(rounded, alignment);
#else
    return std::aligned_alloc(alignment, rounded); // NOLINT(cppcoreguidelines-no-malloc): new
#endif
}

void* checked(void* p) noexcept {
    if (p == nullptr) {
        std::abort();
    }
    return p;
}

void release(void* p) noexcept {
    std::free(p); // NOLINT(cppcoreguidelines-no-malloc): this is operator delete
}

void release_aligned(void* p) noexcept {
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p); // NOLINT(cppcoreguidelines-no-malloc): this is operator delete
#endif
}

} // namespace

std::size_t nepdate_bench::allocation_count() noexcept {
    return g_count.load(std::memory_order_relaxed);
}

std::size_t nepdate_bench::allocation_bytes() noexcept {
    return g_bytes.load(std::memory_order_relaxed);
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
