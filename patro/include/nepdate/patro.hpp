#ifndef NEPDATE_PATRO_HPP
#define NEPDATE_PATRO_HPP

// The patro (PATRO.md): tithi, holidays and events. C4-03 brings the queries of docs/API.md; the
// two name types below already exist because the generated tables (C4-02) hold event objects.

#include <nepdate/date.hpp>         // IWYU pragma: export
#include <nepdate/date_range.hpp>   // IWYU pragma: export
#include <nepdate/patro_config.hpp> // IWYU pragma: export
#include <nepdate/patro_export.hpp> // IWYU pragma: export

#include <cstdint>

namespace nepdate::patro {

namespace detail {
/// A tag for the constructors the generated tables use (docs/API.md: not API).
struct tag {};
} // namespace detail

/// A tithi (lunar day) name; 1 byte, an index into the static name table.
class tithi {
  public:
    /// Builds one from its name table index (generated tables only, not API).
    constexpr tithi(detail::tag /*unused*/, std::uint8_t index) noexcept : index_(index) {}

    friend constexpr bool operator==(tithi, tithi) noexcept = default;

  private:
    std::uint8_t index_;
};

/// One event name; 2 bytes, an index into the static name table.
class event {
  public:
    /// Builds one from its name table index (generated tables only, not API).
    constexpr event(detail::tag /*unused*/, std::uint16_t index) noexcept : index_(index) {}

    friend constexpr bool operator==(event, event) noexcept = default;

  private:
    std::uint16_t index_;
};

} // namespace nepdate::patro

#endif // NEPDATE_PATRO_HPP
