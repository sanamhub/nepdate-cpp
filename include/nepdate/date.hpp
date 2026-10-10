#ifndef NEPDATE_DATE_HPP
#define NEPDATE_DATE_HPP

#include <nepdate/detail/calendar_tables.hpp>
#include <nepdate/detail/civil.hpp>
#include <nepdate/detail/result.hpp> // IWYU pragma: export

#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

namespace nepdate {

/// Language of names and digits: English or Nepali (Devanagari).
enum class lang : std::uint8_t { en, ne };

/// The twelve Bikram Sambat months, Baishakh = 1.
enum class month : std::uint8_t {
    baishakh = 1,
    jestha,
    ashad,
    shrawan,
    bhadra,
    ashoj,
    kartik,
    mangsir,
    poush,
    magh,
    falgun,
    chaitra,
};

class date;
template <> class result<date>;

namespace detail {

// The one way to build a date from its packed value or read that value back. Only code that has
// already validated the value uses it.
struct date_access {
    [[nodiscard]] static constexpr date make(std::uint32_t packed) noexcept;
    [[nodiscard]] static constexpr std::uint32_t packed(date d) noexcept;
};

[[nodiscard]] constexpr std::uint32_t pack(unsigned year, unsigned month, unsigned day) noexcept {
    return year << 16U | month << 8U | day;
}

// Month index (y - 1901) * 12 + (m - 1) into kMonthStart.
[[nodiscard]] constexpr std::uint32_t month_index(date d) noexcept;

// Days since BS 1901-01-01 and back (detail/serial.hpp).
[[nodiscard]] constexpr std::uint32_t to_serial(date d) noexcept;
[[nodiscard]] constexpr date from_serial(std::uint32_t s) noexcept;

} // namespace detail

/// A valid Bikram Sambat date between 1901-01-01 and 2199-12-30. 4 bytes, trivially copyable,
/// ordered chronologically, with no default constructor (D-01).
class date {
  public:
    date() = delete;

    /// Returns BS 1901-01-01.
    [[nodiscard]] static constexpr date min() noexcept { return date{detail::pack(1901, 1, 1)}; }

    /// Returns BS 2199-12-30.
    [[nodiscard]] static constexpr date max() noexcept {
        constexpr auto last_month = static_cast<std::uint32_t>(detail::kMonthStart.size() - 2);
        constexpr std::uint32_t days =
            detail::kMonthStart[last_month + 1] - detail::kMonthStart[last_month];
        return date{detail::pack(static_cast<unsigned>(detail::kMaxYear), 12, days)};
    }

    /// Returns the year, 1901..2199.
    [[nodiscard]] constexpr int year() const noexcept { return static_cast<int>(v_ >> 16U); }

    /// Returns the month.
    [[nodiscard]] constexpr nepdate::month month() const noexcept {
        return static_cast<nepdate::month>((v_ >> 8U) & 0xFFU);
    }

    /// Returns the day of the month, 1..32.
    [[nodiscard]] constexpr int day() const noexcept { return static_cast<int>(v_ & 0xFFU); }

    /// Returns the weekday (ALGORITHM section 7).
    [[nodiscard]] constexpr std::chrono::weekday weekday() const noexcept {
        return std::chrono::weekday{(detail::to_serial(*this) + 4U) % 7U};
    }

    /// Returns the day of the year, 1..367.
    [[nodiscard]] constexpr int day_of_year() const noexcept {
        const std::uint32_t year_start =
            detail::kMonthStart[detail::month_index(*this) / 12U * 12U];
        return static_cast<int>(detail::to_serial(*this) - year_start + 1U);
    }

    /// Returns the length of this date's month, 29..32.
    [[nodiscard]] constexpr int month_length() const noexcept {
        const std::uint32_t i = detail::month_index(*this);
        return static_cast<int>(detail::kMonthStart[i + 1] - detail::kMonthStart[i]);
    }

    /// Returns the length of this date's year, 364..367 (D-12).
    [[nodiscard]] constexpr int year_length() const noexcept {
        const std::uint32_t first = detail::month_index(*this) / 12U * 12U;
        return static_cast<int>(detail::kMonthStart[first + 12] - detail::kMonthStart[first]);
    }

    /// Returns day 1 of this date's month.
    [[nodiscard]] constexpr date first_of_month() const noexcept {
        return date{(v_ & ~0xFFU) | 1U};
    }

    /// Returns the last day of this date's month.
    [[nodiscard]] constexpr date last_of_month() const noexcept {
        return date{(v_ & ~0xFFU) | static_cast<std::uint32_t>(month_length())};
    }

    /// Returns the AD (proleptic Gregorian) date.
    [[nodiscard]] constexpr std::chrono::year_month_day to_year_month_day() const noexcept {
        const detail::civil c = detail::civil_from_days(unix_days());
        return std::chrono::year_month_day{std::chrono::year{c.year}, std::chrono::month{c.month},
                                           std::chrono::day{c.day}};
    }

    /// Returns the AD date as days since 1970-01-01, midnight UTC.
    [[nodiscard]] constexpr std::chrono::sys_days to_sys_days() const noexcept {
        return std::chrono::sys_days{std::chrono::days{unix_days()}};
    }

    /// Returns the date n days later (earlier for a negative n), or out_of_range when that is
    /// outside 1901-01-01..2199-12-30 (D-03).
    [[nodiscard]] constexpr result<date> add_days(int n) const noexcept;

    /// Returns other minus this date in days, negative when other is earlier (D-07).
    [[nodiscard]] constexpr int days_until(date other) const noexcept;

    /// Returns the next day, or std::nullopt at max().
    [[nodiscard]] constexpr std::optional<date> next() const noexcept;

    /// Returns the previous day, or std::nullopt at min().
    [[nodiscard]] constexpr std::optional<date> prev() const noexcept;

    friend constexpr bool operator==(date, date) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(date, date) noexcept = default;

  private:
    friend struct detail::date_access;

    constexpr explicit date(std::uint32_t packed) noexcept : v_(packed) {}

    [[nodiscard]] constexpr int unix_days() const noexcept {
        return static_cast<int>(detail::to_serial(*this)) + detail::kEpochUnixDays;
    }

    // year << 16 | month << 8 | day (ADR-0002 section 1): integer order is chronological order.
    std::uint32_t v_;
};

static_assert(sizeof(date) == 4);
static_assert(std::is_trivially_copyable_v<date>);

constexpr date detail::date_access::make(std::uint32_t packed) noexcept {
    return date{packed};
}

constexpr std::uint32_t detail::date_access::packed(date d) noexcept {
    return d.v_;
}

constexpr std::uint32_t detail::month_index(date d) noexcept {
    const std::uint32_t v = date_access::packed(d);
    return ((v >> 16U) - static_cast<std::uint32_t>(kMinYear)) * 12U + ((v >> 8U) & 0xFFU) - 1U;
}

/// result<date> is 4 bytes (ADR-0002 section 1): every valid date has a year of at least 1901 in
/// the high bits, so an error is stored as its errc value with zero year bits.
template <> class [[nodiscard]] result<date> {
  public:
    constexpr result(date value) noexcept : d_(value) {}
    constexpr result(errc error) noexcept
        : d_(detail::date_access::make(static_cast<std::uint32_t>(error))) {}

    [[nodiscard]] constexpr bool has_value() const noexcept {
        return detail::date_access::packed(d_) > 0xFFU;
    }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    /// Returns the date. On an error: std::abort() at run time, a compile error in a constant
    /// expression.
    [[nodiscard]] constexpr const date& value() const {
        if (!has_value()) {
            detail::bad_result_access();
        }
        return d_;
    }

    /// Returns the date. Precondition: has_value().
    [[nodiscard]] constexpr const date& operator*() const noexcept {
        assert(has_value());
        return d_;
    }

    /// Returns a pointer to the date. Precondition: has_value().
    [[nodiscard]] constexpr const date* operator->() const noexcept {
        assert(has_value());
        return &d_;
    }

    /// Returns the date, or fallback on an error.
    [[nodiscard]] constexpr date value_or(date fallback) const noexcept {
        return has_value() ? d_ : fallback;
    }

    /// Returns the error. Precondition: !has_value().
    [[nodiscard]] constexpr errc error() const noexcept {
        assert(!has_value());
        return static_cast<errc>(detail::date_access::packed(d_) & 0xFFU);
    }

    friend constexpr bool operator==(const result&, const result&) noexcept = default;

  private:
    date d_;
};

static_assert(sizeof(result<date>) == 4);
static_assert(std::is_trivially_copyable_v<result<date>>);

constexpr result<date> date::add_days(int n) const noexcept {
    constexpr int max_serial = static_cast<int>(detail::kMaxSerial);
    // No two dates are further apart than max_serial days. Rejecting a larger n first keeps the
    // sum below from overflowing an int.
    if (n < -max_serial || n > max_serial) {
        return errc::out_of_range;
    }
    const int s = static_cast<int>(detail::to_serial(*this)) + n;
    if (s < 0 || s > max_serial) {
        return errc::out_of_range;
    }
    return detail::from_serial(static_cast<std::uint32_t>(s));
}

constexpr int date::days_until(date other) const noexcept {
    return static_cast<int>(detail::to_serial(other)) - static_cast<int>(detail::to_serial(*this));
}

constexpr std::optional<date> date::next() const noexcept {
    if (*this == max()) {
        return std::nullopt;
    }
    return detail::from_serial(detail::to_serial(*this) + 1U);
}

constexpr std::optional<date> date::prev() const noexcept {
    if (*this == min()) {
        return std::nullopt;
    }
    return detail::from_serial(detail::to_serial(*this) - 1U);
}

/// Returns the BS date year-month-day, or out_of_range (year outside 1901..2199), invalid_month
/// (month outside 1..12) or invalid_day (day outside 1..month length), checked in that order
/// (ALGORITHM section 3).
[[nodiscard]] constexpr result<date> make_date(int year, int month, int day) noexcept {
    // Casting to unsigned first wraps negative and huge values into the rejected range, so no
    // signed arithmetic happens on caller input (ADR-0002 section 3).
    const unsigned y = static_cast<unsigned>(year) - static_cast<unsigned>(detail::kMinYear);
    if (y >= static_cast<unsigned>(detail::kMaxYear - detail::kMinYear + 1)) {
        return errc::out_of_range;
    }
    const unsigned m = static_cast<unsigned>(month) - 1U;
    if (m >= 12U) {
        return errc::invalid_month;
    }
    const unsigned i = y * 12U + m;
    const unsigned d = static_cast<unsigned>(day) - 1U;
    if (d >= detail::kMonthStart[i + 1] - detail::kMonthStart[i]) {
        return errc::invalid_day;
    }
    return detail::date_access::make(detail::pack(y + 1901U, m + 1U, d + 1U));
}

/// Same as make_date(int, int, int) with the month as an enumerator.
[[nodiscard]] constexpr result<date> make_date(int year, nepdate::month m, int day) noexcept {
    return make_date(year, static_cast<int>(m), day);
}

namespace detail {

// Returns the BS date of a Unix day, or out_of_range outside AD 1844-04-11..2143-04-15.
[[nodiscard]] constexpr result<date> from_unix_days(std::int64_t days) noexcept {
    const std::int64_t serial = days - kEpochUnixDays;
    if (serial < 0 || serial > static_cast<std::int64_t>(kMaxSerial)) {
        return errc::out_of_range;
    }
    return from_serial(static_cast<std::uint32_t>(serial));
}

} // namespace detail

/// Returns the BS date of the AD (proleptic Gregorian) date year-month-day: invalid_gregorian when
/// that isn't a real date, out_of_range outside AD 1844-04-11..2143-04-15 (ALGORITHM section 3).
[[nodiscard]] constexpr result<date> from_gregorian(int year, int month, int day) noexcept {
    const unsigned m = static_cast<unsigned>(month) - 1U;
    if (m >= 12U) {
        return errc::invalid_gregorian;
    }
    const unsigned d = static_cast<unsigned>(day) - 1U;
    if (d >= detail::gregorian_month_length(year, m + 1U)) {
        return errc::invalid_gregorian;
    }
    // Reject far-away years before any arithmetic on them.
    if (year < 1844 || year > 2143) {
        return errc::out_of_range;
    }
    return detail::from_unix_days(detail::days_from_civil(year, m + 1U, d + 1U));
}

/// Same as from_gregorian(int, int, int); !ymd.ok() is invalid_gregorian.
[[nodiscard]] constexpr result<date> from_gregorian(std::chrono::year_month_day ymd) noexcept {
    if (!ymd.ok()) {
        return errc::invalid_gregorian;
    }
    return from_gregorian(static_cast<int>(ymd.year()),
                          static_cast<int>(static_cast<unsigned>(ymd.month())),
                          static_cast<int>(static_cast<unsigned>(ymd.day())));
}

/// Returns the BS date of a day count since 1970-01-01, or out_of_range.
[[nodiscard]] constexpr result<date> from_sys_days(std::chrono::sys_days d) noexcept {
    return detail::from_unix_days(static_cast<std::int64_t>(d.time_since_epoch().count()));
}

/// Same as from_sys_days for a local calendar day.
[[nodiscard]] constexpr result<date> from_local_days(std::chrono::local_days d) noexcept {
    return detail::from_unix_days(static_cast<std::int64_t>(d.time_since_epoch().count()));
}

} // namespace nepdate

/// Hashes the 4-byte packed value.
template <> struct std::hash<nepdate::date> {
    [[nodiscard]] std::size_t operator()(nepdate::date d) const noexcept {
        return std::hash<std::uint32_t>{}(nepdate::detail::date_access::packed(d));
    }
};

#include <nepdate/detail/serial.hpp> // IWYU pragma: export

#endif // NEPDATE_DATE_HPP
