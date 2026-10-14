#ifndef NEPDATE_FISCAL_YEAR_HPP
#define NEPDATE_FISCAL_YEAR_HPP

#include <nepdate/date.hpp>
#include <nepdate/date_range.hpp>
#include <nepdate/detail/names.hpp>

#include <array>
#include <charconv>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

namespace nepdate {

/// A quarter of Nepal's fiscal year: Q1 = Shrawan..Ashoj, Q2 = Kartik..Poush, Q3 = Magh..Chaitra,
/// Q4 = Baishakh..Ashad of the next BS year (ALGORITHM section 8b).
enum class quarter : std::uint8_t { q1 = 1, q2, q3, q4 };

/// Returns the fiscal quarter that d's month belongs to.
[[nodiscard]] constexpr quarter quarter_of(date d) noexcept {
    // Shrawan (4) maps to 0, so whole quarters are runs of three from there.
    const auto m = static_cast<unsigned>(d.month());
    return static_cast<quarter>((m + 8U) % 12U / 3U + 1U);
}

/// Returns the three months of q in calendar order. Precondition: q is q1..q4.
[[nodiscard]] constexpr std::array<month, 3> months_of(quarter q) noexcept {
    const unsigned first = 3U * static_cast<unsigned>(q) % 12U + 1U;
    return {static_cast<month>(first), static_cast<month>(first + 1U),
            static_cast<month>(first + 2U)};
}

/// A fiscal year of Nepal, 1 Shrawan to the last day of Ashad (ALGORITHM section 8b, D-08).
/// FY 2082/83 is fiscal_year{2082}. Any int is accepted; members that build a date return
/// out_of_range when it falls outside the calendar.
class fiscal_year {
  public:
    constexpr explicit fiscal_year(int start_year) noexcept : y_(start_year) {}

    /// Returns the fiscal year that contains d: d's year from Shrawan on, the year before until
    /// Ashad.
    [[nodiscard]] static constexpr fiscal_year of(date d) noexcept {
        return fiscal_year{d.month() >= month::shrawan ? d.year() : d.year() - 1};
    }

    /// Returns the BS year in which this fiscal year starts.
    [[nodiscard]] constexpr int start_year() const noexcept { return y_; }

    /// Returns Shrawan 1 of start_year(), or out_of_range.
    [[nodiscard]] constexpr result<date> start() const noexcept { return make_date(y_, 4, 1); }

    /// Returns the last day of Ashad of start_year() + 1, or out_of_range.
    [[nodiscard]] constexpr result<date> end() const noexcept {
        // Also keeps y_ + 1 from overflowing.
        if (y_ >= detail::kMaxYear) {
            return errc::out_of_range;
        }
        const result<date> ashad = make_date(y_ + 1, 3, 1);
        if (!ashad) {
            return ashad;
        }
        return ashad->last_of_month();
    }

    /// Returns start()..end(), or out_of_range when either is outside the calendar.
    [[nodiscard]] constexpr result<date_range> range() const noexcept {
        const result<date> first = start();
        if (!first) {
            return first.error();
        }
        const result<date> last = end();
        if (!last) {
            return last.error();
        }
        return *date_range::make(*first, *last);
    }

    /// Returns the days of quarter q of this fiscal year, or out_of_range when they are outside
    /// the calendar or q isn't q1..q4. Q4 lies in start_year() + 1.
    [[nodiscard]] constexpr result<date_range> quarter_range(quarter q) const noexcept {
        if (static_cast<unsigned>(q) - 1U >= 4U) {
            return errc::out_of_range;
        }
        // Also keeps y_ + 1 from overflowing.
        if (q == quarter::q4 && y_ >= detail::kMaxYear) {
            return errc::out_of_range;
        }
        const int year = q == quarter::q4 ? y_ + 1 : y_;
        const std::array<month, 3> months = months_of(q);
        const result<date> first = make_date(year, months[0], 1);
        if (!first) {
            return first.error();
        }
        return *date_range::make(*first, make_date(year, months[2], 1)->last_of_month());
    }

    /// Writes the label "2082/83" (lang::en) or the same with Devanagari digits (lang::ne) into
    /// [first, last). Returns {end of output, errc{}}, or {last, std::errc::value_too_large} when
    /// the buffer is too small. A negative start year gets a leading '-'.
    [[nodiscard]] constexpr std::to_chars_result to_chars_label(char* first, char* last,
                                                                lang l) const noexcept {
        const std::to_chars_result too_small{last, std::errc::value_too_large};
        if (y_ < 0) {
            if (first == last) {
                return too_small;
            }
            *first++ = '-';
        }
        // Negating in unsigned arithmetic is defined for INT_MIN too.
        const auto magnitude =
            y_ < 0 ? 0U - static_cast<std::uint32_t>(y_) : static_cast<std::uint32_t>(y_);
        char* p = detail::write_number(first, last, magnitude, 1, l);
        if (p == nullptr || p == last) {
            return too_small;
        }
        *p++ = '/';
        // The last two digits of y + 1, in 64 bits so INT_MAX + 1 can't overflow.
        const long long next = static_cast<long long>(y_) + 1;
        const long long two_digits = (next < 0 ? -next : next) % 100;
        p = detail::write_number(p, last, static_cast<std::uint32_t>(two_digits), 2, l);
        if (p == nullptr) {
            return too_small;
        }
        return {p, std::errc{}};
    }

    /// Returns the label that to_chars_label writes. Allocates only the returned string.
    [[nodiscard]] std::string label(lang l) const {
        // '-', 10 digits of 3 bytes, '/', 2 digits of 3 bytes.
        std::array<char, 38> buffer{};
        const std::to_chars_result r =
            to_chars_label(buffer.data(), buffer.data() + buffer.size(), l);
        return std::string(buffer.data(), r.ptr);
    }

    friend constexpr bool operator==(fiscal_year, fiscal_year) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(fiscal_year, fiscal_year) noexcept = default;

  private:
    int y_;
};

} // namespace nepdate

#endif // NEPDATE_FISCAL_YEAR_HPP
