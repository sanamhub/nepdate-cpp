#ifndef NEPDATE_PATRO_HPP
#define NEPDATE_PATRO_HPP

// The patro (PATRO.md): tithi, holidays and events. This package depends on the core; the core
// never mentions it (gate 9). The tables live only in patro.cpp (ADR-0002 section 7).

#include <nepdate/date.hpp>         // IWYU pragma: export
#include <nepdate/date_range.hpp>   // IWYU pragma: export
#include <nepdate/patro_config.hpp> // IWYU pragma: export
#include <nepdate/patro_export.hpp> // IWYU pragma: export

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

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

    /// Returns the tithi's name (PATRO section 2): np for lang::ne, en for lang::en.
    [[nodiscard]] PATRO_EXPORT std::string_view name(lang l) const noexcept;

    friend constexpr bool operator==(tithi, tithi) noexcept = default;

  private:
    std::uint8_t index_;
};

static_assert(sizeof(tithi) == 1);
static_assert(std::is_trivially_copyable_v<tithi>);

/// One event name; 2 bytes, an index into the static name table.
class event {
  public:
    /// Builds one from its name table index (generated tables only, not API).
    constexpr event(detail::tag /*unused*/, std::uint16_t index) noexcept : index_(index) {}

    /// Returns the event's name (PATRO section 2): np for lang::ne, en for lang::en.
    [[nodiscard]] PATRO_EXPORT std::string_view name(lang l) const noexcept;

    friend constexpr bool operator==(event, event) noexcept = default;

  private:
    std::uint16_t index_;
};

static_assert(sizeof(event) == 2);
static_assert(std::is_trivially_copyable_v<event>);

/// What the patro says about one day (ADR-0002 section 7). Trivially copyable; views static data,
/// never owns.
class day_info {
  public:
    /// Returns the day's tithi, or std::nullopt outside the tithi coverage.
    [[nodiscard]] constexpr std::optional<tithi> tithi() const noexcept {
        if (tithi_index_ == 0xFFU) {
            return std::nullopt;
        }
        return nepdate::patro::tithi{detail::tag{}, tithi_index_};
    }

    /// Returns whether the day is a gazetted public holiday.
    [[nodiscard]] constexpr bool is_public_holiday() const noexcept { return holiday_; }

    /// Returns the day's events in file order; empty when none.
    [[nodiscard]] constexpr std::span<const event> events() const noexcept {
        return std::span<const event>{first_, count_};
    }

  private:
    friend PATRO_EXPORT day_info info(date d) noexcept;

    constexpr day_info(detail::tag /*unused*/, const event* first, std::uint16_t count,
                       std::uint8_t tithi_index, bool holiday) noexcept
        : first_(first), count_(count), tithi_index_(tithi_index), holiday_(holiday) {}

    const event* first_ = nullptr;
    std::uint16_t count_ = 0;
    std::uint8_t tithi_index_ = 0xFFU;
    bool holiday_ = false;
};

static_assert(sizeof(day_info) == 16);
static_assert(std::is_trivially_copyable_v<day_info>);

/// Returns what the patro says about d: empty outside the coverage. Never fails. The lookup is a
/// range check, three table reads and one bit test (ADR-0002 section 7).
[[nodiscard]] PATRO_EXPORT day_info info(date d) noexcept;

/// Returns the smallest BS range containing every day with metadata: 2077-01-01..2084-08-20.
[[nodiscard]] PATRO_EXPORT date_range coverage() noexcept;

/// Returns the public holiday dates, ascending (294 today).
[[nodiscard]] PATRO_EXPORT std::span<const date> public_holidays() noexcept;

} // namespace nepdate::patro

// A core whose month lengths differ from the ones the patro tables were built with fails to
// compile instead of returning the wrong day's data (ADR-0002 section 7).
static_assert(nepdate::detail::kCalendarFingerprint == NEPDATE_PATRO_CALENDAR_FINGERPRINT,
              "nepdate-patro was built against a different calendar than this nepdate core");

#endif // NEPDATE_PATRO_HPP
