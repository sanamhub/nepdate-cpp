#ifndef NEPDATE_DATE_RANGE_HPP
#define NEPDATE_DATE_RANGE_HPP

#include <nepdate/date.hpp>

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

namespace nepdate {

class range_set;
class date_interval_view;
class day_filter_view;

/// The inclusive range first..last of dates, first <= last. A random-access view of date that
/// doesn't allocate. The set operations return values or views; the two splits return a vector.
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

    /// Returns the one-day range of d.
    [[nodiscard]] static constexpr date_range single(date d) noexcept { return date_range{d, d}; }

    /// Returns start..start + days - 1, or out_of_range when days is below 1 or the end is outside
    /// the calendar.
    [[nodiscard]] static constexpr result<date_range> from_day_count(date start, int days) noexcept;

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

    /// Returns true when this range fully contains other (an inclusive test).
    [[nodiscard]] constexpr bool contains(date_range other) const noexcept {
        return first_ <= other.first_ && other.last_ <= last_;
    }

    /// Returns true when the two ranges share at least one day.
    [[nodiscard]] constexpr bool overlaps(date_range other) const noexcept {
        return first_ <= other.last_ && other.first_ <= last_;
    }

    /// Returns true when one range starts the day after the other ends. False at the data ends
    /// instead of failing (D-16).
    [[nodiscard]] constexpr bool is_adjacent_to(date_range other) const noexcept;

    /// Returns the shared days, or std::nullopt when the ranges don't overlap (D-01, D-16).
    [[nodiscard]] constexpr std::optional<date_range> intersect(date_range other) const noexcept;

    /// Returns the smallest range covering both.
    [[nodiscard]] constexpr date_range union_with(date_range other) const noexcept;

    /// Returns this range with other removed: no range, one range or two ranges.
    [[nodiscard]] constexpr range_set subtract(date_range other) const noexcept;

    /// Returns one range per month touched, the first and last clipped to this range (D-16).
    [[nodiscard]] std::vector<date_range> split_by_month() const;

    /// Returns one range per fiscal quarter touched, the first and last clipped to this range
    /// (D-16).
    [[nodiscard]] std::vector<date_range> split_by_fiscal_quarter() const;

    /// Returns every interval-th day from first() through last(), or out_of_range when interval is
    /// below 1. The walk stops before the step that would pass last() (D-16).
    [[nodiscard]] constexpr result<date_interval_view>
    dates_with_interval(int interval) const noexcept;

    /// Returns every day except Saturday, and except Sunday when exclude_sunday is true (D-16).
    [[nodiscard]] constexpr day_filter_view
    working_days(bool exclude_sunday = false) const noexcept;

    /// Returns every Saturday, and Sunday when include_sunday is true (D-16).
    [[nodiscard]] constexpr day_filter_view weekend_days(bool include_sunday = true) const noexcept;

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

/// A small fixed container of at most two date ranges, the result of date_range::subtract. It never
/// allocates.
class range_set {
  public:
    /// Visits the ranges in ascending order.
    class const_iterator {
      public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = date_range;
        using difference_type = std::ptrdiff_t;

        constexpr const_iterator() noexcept = default;

        [[nodiscard]] constexpr date_range operator*() const noexcept {
            return *items_[index_]; // NOLINT(bugprone-unchecked-optional-access): index_ < size
        }

        constexpr const_iterator& operator++() noexcept {
            ++index_;
            return *this;
        }
        // NOLINTNEXTLINE(cert-dcl21-cpp): postfix ++ must return the old iterator by value
        constexpr const_iterator operator++(int) noexcept {
            const const_iterator old = *this;
            ++index_;
            return old;
        }

        friend constexpr bool operator==(const const_iterator& a,
                                         const const_iterator& b) noexcept {
            return a.items_ == b.items_ && a.index_ == b.index_;
        }

      private:
        friend class range_set;
        constexpr const_iterator(const std::optional<date_range>* items, std::size_t index) noexcept
            : items_(items), index_(index) {}

        const std::optional<date_range>* items_ = nullptr;
        std::size_t index_ = 0;
    };

    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    /// Returns the range at i. Precondition: i < size().
    [[nodiscard]] constexpr date_range operator[](std::size_t i) const noexcept {
        return *items_[i]; // NOLINT(bugprone-unchecked-optional-access): i < size_
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator{items_.data(), 0};
    }
    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator{items_.data(), size_};
    }

  private:
    friend class date_range;
    constexpr void push(date_range r) noexcept {
        items_[size_] = r;
        ++size_;
    }

    std::array<std::optional<date_range>, 2> items_{};
    std::size_t size_ = 0;
};

/// The dates first(), first() + step, ... through last(), as a random-access view that doesn't
/// allocate. date_range::dates_with_interval builds it.
class date_interval_view : public std::ranges::view_interface<date_interval_view> {
  public:
    /// Random-access iterator; step and first day are held by value.
    class iterator {
      public:
        using iterator_concept = std::random_access_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = date;
        using difference_type = std::ptrdiff_t;

        constexpr iterator() noexcept = default;

        [[nodiscard]] constexpr date operator*() const noexcept {
            const auto offset =
                static_cast<std::uint32_t>(static_cast<std::uint64_t>(index_) * step_);
            return detail::from_serial(first_ + offset);
        }
        [[nodiscard]] constexpr date operator[](difference_type n) const noexcept {
            return *(*this + n);
        }

        constexpr iterator& operator++() noexcept {
            ++index_;
            return *this;
        }
        // NOLINTNEXTLINE(cert-dcl21-cpp): postfix ++ must return the old iterator by value
        constexpr iterator operator++(int) noexcept {
            const iterator old = *this;
            ++index_;
            return old;
        }
        constexpr iterator& operator--() noexcept {
            --index_;
            return *this;
        }
        // NOLINTNEXTLINE(cert-dcl21-cpp): postfix -- must return the old iterator by value
        constexpr iterator operator--(int) noexcept {
            const iterator old = *this;
            --index_;
            return old;
        }
        constexpr iterator& operator+=(difference_type n) noexcept {
            index_ += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) noexcept {
            index_ -= n;
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
            return a.index_ - b.index_;
        }

        friend constexpr bool operator==(iterator, iterator) noexcept = default;
        friend constexpr std::strong_ordering operator<=>(iterator, iterator) noexcept = default;

      private:
        friend class date_interval_view;
        constexpr iterator(std::uint32_t first, std::uint32_t step, difference_type index) noexcept
            : first_(first), step_(step), index_(index) {}

        std::uint32_t first_ = 0;
        std::uint32_t step_ = 1;
        difference_type index_ = 0;
    };

    constexpr date_interval_view() noexcept = default;

    [[nodiscard]] constexpr date first() const noexcept { return detail::from_serial(first_); }
    [[nodiscard]] constexpr date last() const noexcept { return detail::from_serial(last_); }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return count_; }

    [[nodiscard]] constexpr iterator begin() const noexcept { return iterator{first_, step_, 0}; }
    [[nodiscard]] constexpr iterator end() const noexcept {
        return iterator{first_, step_, static_cast<std::ptrdiff_t>(count_)};
    }

  private:
    friend class date_range;
    constexpr date_interval_view(date first, date last, std::uint32_t step) noexcept
        : first_(detail::to_serial(first)), last_(detail::to_serial(last)), step_(step),
          count_((detail::to_serial(last) - detail::to_serial(first)) / step + 1U) {}

    std::uint32_t first_ = 0;
    std::uint32_t last_ = 0;
    std::uint32_t step_ = 1;
    std::uint32_t count_ = 1;
};

/// Every working day (or weekend day) of a range, as a bidirectional view that doesn't allocate.
/// date_range::working_days and date_range::weekend_days build it.
class day_filter_view : public std::ranges::view_interface<day_filter_view> {
  public:
    /// Which days the view keeps.
    enum class kind : std::uint8_t { working, weekend };

    /// Bidirectional iterator that skips the days the view doesn't keep.
    class iterator {
      public:
        using iterator_concept = std::bidirectional_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = date;
        using difference_type = std::ptrdiff_t;

        constexpr iterator() noexcept = default;

        [[nodiscard]] constexpr date operator*() const noexcept {
            return detail::from_serial(pos_);
        }

        constexpr iterator& operator++() noexcept {
            if (pos_ <= last_) {
                ++pos_;
                while (pos_ <= last_ && !matches(pos_)) {
                    ++pos_;
                }
            }
            return *this;
        }
        // NOLINTNEXTLINE(cert-dcl21-cpp): postfix ++ must return the old iterator by value
        constexpr iterator operator++(int) noexcept {
            const iterator old = *this;
            ++*this;
            return old;
        }
        constexpr iterator& operator--() noexcept {
            if (pos_ > last_) {
                pos_ = last_;
            } else if (pos_ > first_) {
                --pos_;
            }
            while (pos_ > first_ && !matches(pos_)) {
                --pos_;
            }
            return *this;
        }
        // NOLINTNEXTLINE(cert-dcl21-cpp): postfix -- must return the old iterator by value
        constexpr iterator operator--(int) noexcept {
            const iterator old = *this;
            --*this;
            return old;
        }

        friend constexpr bool operator==(iterator, iterator) noexcept = default;

      private:
        friend class day_filter_view;
        constexpr iterator(std::uint32_t first, std::uint32_t last, kind k, bool sunday,
                           std::uint32_t pos) noexcept
            : first_(first), last_(last), pos_(pos), kind_(k), sunday_(sunday) {}

        [[nodiscard]] constexpr bool matches(std::uint32_t s) const noexcept {
            // The serial of 1901-01-01 is a Thursday, which is weekday 4; Sunday is 0.
            const unsigned wd = (s + 4U) % 7U;
            if (kind_ == kind::working) {
                if (wd == 6U) {
                    return false;
                }
                return !sunday_ || wd != 0U;
            }
            return wd == 6U || (sunday_ && wd == 0U);
        }

        std::uint32_t first_ = 0;
        std::uint32_t last_ = 0;
        std::uint32_t pos_ = 0;
        kind kind_ = kind::working;
        bool sunday_ = false;
    };

    constexpr day_filter_view() noexcept = default;

    [[nodiscard]] constexpr iterator begin() const noexcept {
        iterator it{first_, last_, kind_, sunday_, first_};
        if (!it.matches(it.pos_)) {
            ++it;
        }
        return it;
    }
    [[nodiscard]] constexpr iterator end() const noexcept {
        return iterator{first_, last_, kind_, sunday_, last_ + 1U};
    }

    /// Returns the number of kept days.
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        std::size_t n = 0;
        for (iterator it = begin(); it != end(); ++it) {
            ++n;
        }
        return n;
    }

  private:
    friend class date_range;
    constexpr day_filter_view(date first, date last, kind k, bool sunday) noexcept
        : first_(detail::to_serial(first)), last_(detail::to_serial(last)), kind_(k),
          sunday_(sunday) {}

    std::uint32_t first_ = 0;
    std::uint32_t last_ = 0;
    kind kind_ = kind::working;
    bool sunday_ = false;
};

static_assert(sizeof(date_range) == 8);
static_assert(std::is_trivially_copyable_v<date_range>);
static_assert(sizeof(date_interval_view) <= 16);
static_assert(std::is_trivially_copyable_v<date_interval_view>);
static_assert(std::is_trivially_copyable_v<range_set>);
static_assert(std::bidirectional_iterator<day_filter_view::iterator>);

constexpr result<date_range> date_range::from_day_count(date start, int days) noexcept {
    if (days < 1) {
        return errc::out_of_range;
    }
    // days - 1 can't overflow after the check above, and add_days rejects anything past max().
    const result<date> end = start.add_days(days - 1);
    if (!end) {
        return end.error();
    }
    return date_range{start, *end};
}

constexpr bool date_range::is_adjacent_to(date_range other) const noexcept {
    const result<date> after = last_.add_days(1);
    if (after.has_value() && *after == other.first_) {
        return true;
    }
    const result<date> other_after = other.last_.add_days(1);
    return other_after.has_value() && *other_after == first_;
}

constexpr std::optional<date_range> date_range::intersect(date_range other) const noexcept {
    if (!overlaps(other)) {
        return std::nullopt;
    }
    const date first = first_ < other.first_ ? other.first_ : first_;
    const date last = last_ < other.last_ ? last_ : other.last_;
    return date_range{first, last};
}

constexpr date_range date_range::union_with(date_range other) const noexcept {
    return date_range{first_ < other.first_ ? first_ : other.first_,
                      last_ < other.last_ ? other.last_ : last_};
}

constexpr range_set date_range::subtract(date_range other) const noexcept {
    range_set out;
    if (!overlaps(other)) {
        out.push(*this);
        return out;
    }
    if (first_ < other.first_) {
        const std::optional<date> before = other.first_.prev();
        if (before.has_value()) {
            out.push(date_range{first_, *before});
        }
    }
    if (other.last_ < last_) {
        const std::optional<date> after = other.last_.next();
        if (after.has_value()) {
            out.push(date_range{*after, last_});
        }
    }
    return out;
}

inline std::vector<date_range> date_range::split_by_month() const {
    std::vector<date_range> pieces;
    date current = first_;
    for (;;) {
        const date month_end = current.last_of_month();
        const date piece_end = month_end < last_ ? month_end : last_;
        pieces.push_back(date_range{current, piece_end});
        if (piece_end == last_) {
            break;
        }
        const std::optional<date> next = piece_end.next();
        if (!next.has_value()) {
            break;
        }
        current = *next;
    }
    return pieces;
}

inline std::vector<date_range> date_range::split_by_fiscal_quarter() const {
    std::vector<date_range> pieces;
    date current = first_;
    for (;;) {
        const int month_no = static_cast<int>(current.month());
        // Shrawan (4) starts the fiscal year; Baishakh to Ashad (1 to 3) belong to the year before.
        const int fiscal_year = month_no >= 4 ? current.year() : current.year() - 1;
        const int quarter_index = (month_no + 8) % 12 / 3; // 0..3 for Q1..Q4
        // Q1 ends in Ashoj (6), Q2 in Poush (9), Q3 in Chaitra (12), Q4 in Ashad (3).
        const int end_month = quarter_index < 3 ? 6 + 3 * quarter_index : 3;
        const int end_year = quarter_index < 3 ? fiscal_year : fiscal_year + 1;
        const result<date> end_first = make_date(end_year, end_month, 1);
        if (!end_first) {
            break; // unreachable for a valid current date
        }
        const date quarter_end = end_first->last_of_month();
        const date piece_end = quarter_end < last_ ? quarter_end : last_;
        pieces.push_back(date_range{current, piece_end});
        if (piece_end == last_) {
            break;
        }
        const std::optional<date> next = piece_end.next();
        if (!next.has_value()) {
            break;
        }
        current = *next;
    }
    return pieces;
}

constexpr result<date_interval_view> date_range::dates_with_interval(int interval) const noexcept {
    if (interval < 1) {
        return errc::out_of_range;
    }
    return date_interval_view{first_, last_, static_cast<std::uint32_t>(interval)};
}

constexpr day_filter_view date_range::working_days(bool exclude_sunday) const noexcept {
    return day_filter_view{first_, last_, day_filter_view::kind::working, exclude_sunday};
}

constexpr day_filter_view date_range::weekend_days(bool include_sunday) const noexcept {
    return day_filter_view{first_, last_, day_filter_view::kind::weekend, include_sunday};
}

/// Returns today's date in the zone with utc_offset in month m of that month, or out_of_range when
/// today or the month cannot be built.
[[nodiscard]] inline result<date_range>
current_month(std::chrono::minutes utc_offset = nepal_utc_offset) noexcept {
    const result<date> today_date = today(utc_offset);
    if (!today_date) {
        return today_date.error();
    }
    return date_range::of_month(today_date->year(), today_date->month());
}

/// Same as current_month(minutes) with an injected clock.
template <class Clock>
[[nodiscard]] result<date_range> current_month(std::chrono::minutes utc_offset,
                                               Clock clock) noexcept {
    const result<date> today_date = today(utc_offset, clock);
    if (!today_date) {
        return today_date.error();
    }
    return date_range::of_month(today_date->year(), today_date->month());
}

/// Returns the fiscal year that contains today in the zone with utc_offset, or out_of_range when
/// it reaches past the calendar.
[[nodiscard]] inline result<date_range>
current_fiscal_year(std::chrono::minutes utc_offset = nepal_utc_offset) noexcept {
    const result<date> today_date = today(utc_offset);
    if (!today_date) {
        return today_date.error();
    }
    const int start_year =
        today_date->month() >= month::shrawan ? today_date->year() : today_date->year() - 1;
    const result<date> first = make_date(start_year, 4, 1);
    if (!first) {
        return first.error();
    }
    const result<date> last = make_date(start_year + 1, 3, 1);
    if (!last) {
        return last.error();
    }
    const std::optional<date_range> range = date_range::make(*first, last->last_of_month());
    if (!range.has_value()) {
        return errc::out_of_range;
    }
    return *range;
}

/// Same as current_fiscal_year(minutes) with an injected clock.
template <class Clock>
[[nodiscard]] result<date_range> current_fiscal_year(std::chrono::minutes utc_offset,
                                                     Clock clock) noexcept {
    const result<date> today_date = today(utc_offset, clock);
    if (!today_date) {
        return today_date.error();
    }
    const int start_year =
        today_date->month() >= month::shrawan ? today_date->year() : today_date->year() - 1;
    const result<date> first = make_date(start_year, 4, 1);
    if (!first) {
        return first.error();
    }
    const result<date> last = make_date(start_year + 1, 3, 1);
    if (!last) {
        return last.error();
    }
    const std::optional<date_range> range = date_range::make(*first, last->last_of_month());
    if (!range.has_value()) {
        return errc::out_of_range;
    }
    return *range;
}

/// Returns the BS calendar year that contains today in the zone with utc_offset.
[[nodiscard]] inline result<date_range>
current_calendar_year(std::chrono::minutes utc_offset = nepal_utc_offset) noexcept {
    const result<date> today_date = today(utc_offset);
    if (!today_date) {
        return today_date.error();
    }
    return date_range::of_year(today_date->year());
}

/// Same as current_calendar_year(minutes) with an injected clock.
template <class Clock>
[[nodiscard]] result<date_range> current_calendar_year(std::chrono::minutes utc_offset,
                                                       Clock clock) noexcept {
    const result<date> today_date = today(utc_offset, clock);
    if (!today_date) {
        return today_date.error();
    }
    return date_range::of_year(today_date->year());
}

} // namespace nepdate

// A date_range holds its two dates by value and its iterators hold serials, so an iterator outlives
// the range it came from. The two views hold their own bounds and iterators likewise.
// NOLINTBEGIN(cert-dcl58-cpp): enable_borrowed_range is the standard range customization point
namespace std::ranges {
template <> inline constexpr bool enable_borrowed_range<nepdate::date_range> = true;
template <> inline constexpr bool enable_borrowed_range<nepdate::date_interval_view> = true;
template <> inline constexpr bool enable_borrowed_range<nepdate::day_filter_view> = true;
} // namespace std::ranges
// NOLINTEND(cert-dcl58-cpp)

/// Hashes both bounds.
template <> struct std::hash<nepdate::date_range> {
    [[nodiscard]] std::size_t operator()(nepdate::date_range r) const noexcept {
        const std::size_t first = std::hash<nepdate::date>{}(r.first());
        const std::size_t last = std::hash<nepdate::date>{}(r.last());
        return first ^ (last + 0x9e3779b9U + (first << 6U) + (first >> 2U));
    }
};

#endif // NEPDATE_DATE_RANGE_HPP
