#ifndef NEPDATE_DATE_RANGE_HPP
#define NEPDATE_DATE_RANGE_HPP

#include <nepdate/date.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>

namespace nepdate {

/// The inclusive range first..last of dates, first <= last. A random-access view of date that
/// doesn't allocate (D-09: no set operations, splitting or working days).
class date_range : public std::ranges::view_interface<date_range> {
  public:
    /// Random-access iterator over the range. Holds a serial day and converts it to a date on
    /// dereference, so operator* returns a date by value.
    class iterator {
      public:
        using iterator_concept = std::random_access_iterator_tag;
        // operator* returns a prvalue, which a C++17 forward iterator may not do.
        using iterator_category = std::input_iterator_tag;
        using value_type = date;
        using difference_type = std::ptrdiff_t;

        constexpr iterator() noexcept = default;

        [[nodiscard]] constexpr date operator*() const noexcept { return detail::from_serial(s_); }
        [[nodiscard]] constexpr date operator[](difference_type n) const noexcept {
            return *(*this + n);
        }

        constexpr iterator& operator++() noexcept {
            ++s_;
            return *this;
        }
        // NOLINTBEGIN(cert-dcl21-cpp): postfix ++/-- must return the old iterator by value (std::random_access_iterator)
        constexpr iterator operator++(int) noexcept {
            const iterator old = *this;
            ++s_;
            return old;
        }
        constexpr iterator& operator--() noexcept {
            --s_;
            return *this;
        }
        constexpr iterator operator--(int) noexcept {
            const iterator old = *this;
            --s_;
            return old;
        }
        // NOLINTEND(cert-dcl21-cpp)
        constexpr iterator& operator+=(difference_type n) noexcept {
            // Unsigned wrap-around gives the right serial for negative n as well.
            s_ += static_cast<std::uint32_t>(n);
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) noexcept {
            s_ -= static_cast<std::uint32_t>(n);
            return *this;
        }

        [[nodiscard]] friend constexpr iterator operator+(iterator i, difference_type n) noexcept {
            return i += n;
        }
        [[nodiscard]] friend constexpr iterator operator+(difference_type n, iterator i) noexcept {
            return i += n;
        }
        [[nodiscard]] friend constexpr iterator operator-(iterator i, difference_type n) noexcept {
            return i -= n;
        }
        [[nodiscard]] friend constexpr difference_type operator-(iterator a, iterator b) noexcept {
            return static_cast<difference_type>(a.s_) - static_cast<difference_type>(b.s_);
        }

        friend constexpr bool operator==(iterator, iterator) noexcept = default;
        friend constexpr std::strong_ordering operator<=>(iterator, iterator) noexcept = default;

      private:
        friend class date_range;
        constexpr explicit iterator(std::uint32_t s) noexcept : s_(s) {}

        // Serial day; end() holds one past the last day, at most kMaxSerial + 1.
        std::uint32_t s_ = 0;
    };

    /// Returns the range first..last, or std::nullopt when first > last.
    [[nodiscard]] static constexpr std::optional<date_range> make(date first, date last) noexcept {
        if (last < first) {
            return std::nullopt;
        }
        return date_range{first, last};
    }

    /// Returns every day of month m of year, or out_of_range (year outside 1901..2199) or
    /// invalid_month.
    [[nodiscard]] static constexpr result<date_range> of_month(int year, month m) noexcept {
        const result<date> first = make_date(year, m, 1);
        if (!first) {
            return first.error();
        }
        return date_range{*first, first->last_of_month()};
    }

    /// Returns every day of year, Baishakh 1 to the last day of Chaitra, or out_of_range.
    [[nodiscard]] static constexpr result<date_range> of_year(int year) noexcept {
        const result<date> first = make_date(year, 1, 1);
        if (!first) {
            return first.error();
        }
        return date_range{*first, make_date(year, 12, 1)->last_of_month()};
    }

    /// Returns the first day.
    [[nodiscard]] constexpr date first() const noexcept { return first_; }

    /// Returns the last day.
    [[nodiscard]] constexpr date last() const noexcept { return last_; }

    /// Returns the number of days, at least 1.
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return static_cast<std::size_t>(detail::to_serial(last_) - detail::to_serial(first_)) + 1U;
    }

    /// Returns true when first() <= d <= last().
    [[nodiscard]] constexpr bool contains(date d) const noexcept {
        return first_ <= d && d <= last_;
    }

    [[nodiscard]] constexpr iterator begin() const noexcept {
        return iterator{detail::to_serial(first_)};
    }
    [[nodiscard]] constexpr iterator end() const noexcept {
        return iterator{detail::to_serial(last_) + 1U};
    }

    // Written out: a defaulted == would also compare the view_interface base, which has no ==.
    friend constexpr bool operator==(date_range a, date_range b) noexcept {
        return a.first_ == b.first_ && a.last_ == b.last_;
    }

  private:
    constexpr date_range(date first, date last) noexcept : first_(first), last_(last) {}

    date first_;
    date last_;
};

} // namespace nepdate

// A date_range holds its two dates by value and its iterators hold serials, so an iterator
// outlives the range it came from.
// NOLINTNEXTLINE(cert-dcl58-cpp): enable_borrowed_range is the standard range customization point
namespace std::ranges {
template <> inline constexpr bool enable_borrowed_range<nepdate::date_range> = true;
} // namespace std::ranges

#endif // NEPDATE_DATE_RANGE_HPP
