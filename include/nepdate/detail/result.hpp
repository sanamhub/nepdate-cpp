#ifndef NEPDATE_DETAIL_RESULT_HPP
#define NEPDATE_DETAIL_RESULT_HPP

// nepdate::errc and nepdate::result<T> (ADR-0003 section 2). Include <nepdate/date.hpp>, which also
// defines the 4-byte result<date>.

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <type_traits>

namespace nepdate {

/// Why an operation failed. The first four come from validation (ALGORITHM section 3), the last
/// five from parsing (PARSING section 4). The value 0 is never an error.
enum class errc : std::uint8_t {
    out_of_range = 1,  ///< outside BS 1901-01-01..2199-12-30 or AD 1844-04-11..2143-04-15
    invalid_month,     ///< month not 1..12
    invalid_day,       ///< day not 1..month length
    invalid_gregorian, ///< not a real proleptic Gregorian date
    invalid_character,
    wrong_group_count,
    number_too_long,
    ambiguous,
    unrecognized,
};

/// Returns the enumerator's name, for example "out_of_range", or "" for a value that isn't one.
[[nodiscard]] constexpr std::string_view to_string(errc e) noexcept {
    switch (e) {
    case errc::out_of_range:
        return "out_of_range";
    case errc::invalid_month:
        return "invalid_month";
    case errc::invalid_day:
        return "invalid_day";
    case errc::invalid_gregorian:
        return "invalid_gregorian";
    case errc::invalid_character:
        return "invalid_character";
    case errc::wrong_group_count:
        return "wrong_group_count";
    case errc::number_too_long:
        return "number_too_long";
    case errc::ambiguous:
        return "ambiguous";
    case errc::unrecognized:
        return "unrecognized";
    }
    return {};
}

/// Returns true for the five parse kinds, invalid_character to unrecognized.
[[nodiscard]] constexpr bool is_parse_error(errc e) noexcept {
    return e >= errc::invalid_character && e <= errc::unrecognized;
}

namespace detail {

// Not constexpr on purpose: reaching it in a constant expression makes that expression ill-formed,
// so `constexpr auto d = make_date(2081, 4, 33).value();` fails to compile (ADR-0003 section 2).
[[noreturn]] inline void bad_result_access() noexcept {
    std::abort();
}

} // namespace detail

/// Holds a T or an errc, never both (ADR-0003 section 2). Member names follow std::expected.
/// result<date> is specialised in <nepdate/date.hpp> to 4 bytes.
template <class T> class [[nodiscard]] result {
    static_assert(std::is_trivially_copyable_v<T>, "result<T> needs a trivially copyable T");
    // The rvalue accessors return T by value (ADR-0006); a larger T would make that copy expensive.
    static_assert(sizeof(T) <= 16,
                  "result<T> rvalue accessors copy T; ADR-0006 caps T at 16 bytes");

  public:
    // Implicit, so `return d;` and `return errc::out_of_range;` work.
    constexpr result(T value) noexcept : value_(value) {}
    // empty_ is the active member on an error, so a constexpr error result is fully initialised
    // (GCC 14.3+ rejects a constant whose union has no active member).
    constexpr result(errc error) noexcept : empty_{}, error_(error) {}

    /// Returns true when the result holds a value.
    [[nodiscard]] constexpr bool has_value() const noexcept { return error_ == errc{}; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    /// Returns the value. On an error: std::abort() at run time, a compile error in a constant
    /// expression.
    [[nodiscard]] constexpr const T& value() const& {
        if (!has_value()) {
            detail::bad_result_access();
        }
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): checked just above
    }

    /// Returns the value by copy, so a range-for over a temporary's value is safe in C++20
    /// (ADR-0006). On an error: std::abort() at run time, a compile error in a constant expression.
    [[nodiscard]] constexpr T value() && {
        if (!has_value()) {
            detail::bad_result_access();
        }
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): checked just above
    }

    /// Same as value() && for a const rvalue.
    [[nodiscard]] constexpr T value() const&& {
        if (!has_value()) {
            detail::bad_result_access();
        }
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): checked just above
    }

    /// Returns the value. Precondition: has_value().
    [[nodiscard]] constexpr const T& operator*() const& noexcept {
        assert(has_value());
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): precondition
    }

    /// Returns the value by copy, so `*of_year(y)` as a range-for range is safe in C++20
    /// (ADR-0006). Precondition: has_value().
    [[nodiscard]] constexpr T operator*() && noexcept {
        assert(has_value());
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): precondition
    }

    /// Same as operator*() && for a const rvalue, which would otherwise bind the const& overload.
    [[nodiscard]] constexpr T operator*() const&& noexcept {
        assert(has_value());
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): precondition
    }

    /// Returns a pointer to the value. Precondition: has_value().
    [[nodiscard]] constexpr const T* operator->() const noexcept {
        assert(has_value());
        return &value_; // NOLINT(cppcoreguidelines-pro-type-union-access): precondition
    }

    /// Returns the value, or fallback on an error.
    [[nodiscard]] constexpr T value_or(T fallback) const noexcept {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): has_value() picks the member
        return has_value() ? value_ : fallback;
    }

    /// Returns the error. Precondition: !has_value().
    [[nodiscard]] constexpr errc error() const noexcept {
        assert(!has_value());
        return error_;
    }

    // Not defaulted: a defaulted == can't compare the union.
    friend constexpr bool operator==(const result& a, const result& b) noexcept {
        if (a.has_value() != b.has_value()) {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): both hold a value here
        return a.has_value() ? a.value_ == b.value_ : a.error_ == b.error_;
    }

  private:
    struct empty {};
    // A union, so T needs no default constructor (date has none, D-01). empty_ is active when
    // error_ is set, value_ otherwise.
    union {
        empty empty_;
        T value_;
    };
    errc error_{};
};

} // namespace nepdate

#endif // NEPDATE_DETAIL_RESULT_HPP
