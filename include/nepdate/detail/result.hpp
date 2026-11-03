#ifndef NEPDATE_DETAIL_RESULT_HPP
#define NEPDATE_DETAIL_RESULT_HPP

// nepdate::errc and nepdate::result<T> (ADR-0003 section 2). Include <nepdate/date.hpp>, which also
// defines the 4-byte result<date>.

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

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

/// Thrown by result<T>::value() when the result holds an error.
class bad_result_access : public std::exception {
  public:
    /// Returns "nepdate: result holds an error".
    [[nodiscard]] const char* what() const noexcept override {
        return "nepdate: result holds an error";
    }
};

namespace detail {

// Not constexpr on purpose: reaching it in a constant expression makes that expression ill-formed,
// so `constexpr auto d = make_date(2081, 4, 33).value();` fails to compile (ADR-0003 section 2).
// A build without exceptions aborts instead, so the library still works with -fno-exceptions.
[[noreturn]] inline void throw_bad_result_access() {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    throw bad_result_access{};
#else
    std::abort();
#endif
}

// The category behind std::error_code and std::error_condition for errc.
class errc_category final : public std::error_category {
  public:
    [[nodiscard]] const char* name() const noexcept override { return "nepdate"; }

    [[nodiscard]] std::string message(int condition) const override {
        const std::string_view text = to_string(static_cast<errc>(condition));
        if (text.empty()) {
            return "unknown nepdate error";
        }
        return std::string(text);
    }
};

} // namespace detail

/// Returns the std::error_category shared by every errc value.
[[nodiscard]] inline const std::error_category& errc_category() noexcept {
    static const detail::errc_category category;
    return category;
}

/// Returns e as a std::error_code, so standard error handling prints the code's name.
[[nodiscard]] inline std::error_code make_error_code(errc e) noexcept {
    return {static_cast<int>(e), errc_category()};
}

/// Returns e as a std::error_condition, for comparisons with the standard predicates.
[[nodiscard]] inline std::error_condition make_error_condition(errc e) noexcept {
    return {static_cast<int>(e), errc_category()};
}

/// Holds a T or an errc, never both (ADR-0003 section 2). Member names follow std::expected.
/// result<date> is specialised in <nepdate/date.hpp> to 4 bytes. Small trivially copyable values
/// use the compact union; a larger or non-trivial value, such as a std::vector, uses optional
/// storage in the second specialization below.
template <class T, class = void> class [[nodiscard]] result;

namespace detail {

// The compact union result needs a small trivially copyable T (ADR-0006).
template <class T>
inline constexpr bool kCompactResult = std::is_trivially_copyable_v<T> && sizeof(T) <= 16;

} // namespace detail

/// The compact result for a small trivially copyable T (ADR-0006).
template <class T> class [[nodiscard]] result<T, std::enable_if_t<detail::kCompactResult<T>>> {
  public:
    // Implicit, so `return d;` and `return errc::out_of_range;` work.
    constexpr result(T value) noexcept : value_(value) {}
    // empty_ is the active member on an error, so a constexpr error result is fully initialised
    // (GCC 14.3+ rejects a constant whose union has no active member).
    constexpr result(errc error) noexcept : empty_{}, error_(error) {}

    /// Returns true when the result holds a value.
    [[nodiscard]] constexpr bool has_value() const noexcept { return error_ == errc{}; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    /// Returns the value. On an error: throws bad_result_access, or std::abort() without
    /// exceptions. In a constant expression that path is a compile error.
    [[nodiscard]] constexpr const T& value() const& {
        if (!has_value()) {
            detail::throw_bad_result_access();
        }
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): checked just above
    }

    /// Returns the value by copy, so a range-for over a temporary's value is safe in C++20
    /// (ADR-0006). On an error: throws bad_result_access, or std::abort() without exceptions.
    [[nodiscard]] constexpr T value() && {
        if (!has_value()) {
            detail::throw_bad_result_access();
        }
        return value_; // NOLINT(cppcoreguidelines-pro-type-union-access): checked just above
    }

    /// Same as value() && for a const rvalue.
    [[nodiscard]] constexpr T value() const&& {
        if (!has_value()) {
            detail::throw_bad_result_access();
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

/// The result for a T that is too large or not trivially copyable, such as a std::vector. It owns
/// the value through std::optional, so it allocates only when T does.
template <class T> class [[nodiscard]] result<T, std::enable_if_t<!detail::kCompactResult<T>>> {
  public:
    // Implicit, so `return values;` and `return errc::out_of_range;` work.
    result(T value) : value_(std::move(value)) {}
    result(errc error) noexcept : error_(error) {}

    /// Returns true when the result holds a value.
    [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    /// Returns the value. On an error: throws bad_result_access, or std::abort() without
    /// exceptions.
    [[nodiscard]] const T& value() const& {
        if (!has_value()) {
            detail::throw_bad_result_access();
        }
        return *value_;
    }

    /// Returns the value by move, so a range-for over a temporary's value is safe in C++20.
    [[nodiscard]] T value() && {
        if (!has_value()) {
            detail::throw_bad_result_access();
        }
        return std::move(*value_);
    }

    /// Same as value() && for a const rvalue.
    [[nodiscard]] T value() const&& {
        if (!has_value()) {
            detail::throw_bad_result_access();
        }
        return *value_;
    }

    /// Returns the value. Precondition: has_value().
    [[nodiscard]] const T& operator*() const& noexcept {
        assert(has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): precondition has_value()
        return *value_;
    }

    /// Returns the value by move. Precondition: has_value().
    [[nodiscard]] T operator*() && {
        assert(has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): precondition has_value()
        return std::move(*value_);
    }

    /// Same as operator*() && for a const rvalue.
    [[nodiscard]] T operator*() const&& {
        assert(has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): precondition has_value()
        return *value_;
    }

    /// Returns a pointer to the value. Precondition: has_value().
    [[nodiscard]] const T* operator->() const noexcept {
        assert(has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): precondition has_value()
        return &*value_;
    }

    /// Returns the value, or fallback on an error.
    [[nodiscard]] T value_or(T fallback) const {
        return has_value() ? *value_ : std::move(fallback);
    }

    /// Returns the error. Precondition: !has_value().
    [[nodiscard]] errc error() const noexcept {
        assert(!has_value());
        return error_;
    }

    friend bool operator==(const result& a, const result& b) {
        if (a.has_value() != b.has_value()) {
            return false;
        }
        return a.has_value() ? *a.value_ == *b.value_ : a.error_ == b.error_;
    }

  private:
    std::optional<T> value_;
    errc error_{};
};

} // namespace nepdate

// Makes `std::error_code ec = nepdate::errc::out_of_range;` and `ec == nepdate::errc::x` compile
// through nepdate::make_error_code. Only one implicit conversion is enabled, because specializing
// both enums would make that comparison ambiguous; an error condition is made explicitly with
// nepdate::make_error_condition.
template <> struct std::is_error_code_enum<nepdate::errc> : std::true_type {};

#endif // NEPDATE_DETAIL_RESULT_HPP
