#ifndef NEPDATE_TESTS_FUZZ_PROPERTY_HPP
#define NEPDATE_TESTS_FUZZ_PROPERTY_HPP

// C3-07: the property every parse input must satisfy (ADR-0004 section 3), shared by the two
// libFuzzer targets and the corpus CTest test. Test code, not part of the library.

#include <nepdate/format.hpp>
#include <nepdate/parse.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <system_error>

namespace nepdate_test::fuzz {

/// True when e is one of the eight kinds PARSING section 4 can produce. invalid_gregorian is the
/// only errc parse never returns.
[[nodiscard]] constexpr bool is_parse_kind(nepdate::errc e) noexcept {
    switch (e) {
    case nepdate::errc::out_of_range:
    case nepdate::errc::invalid_month:
    case nepdate::errc::invalid_day:
    case nepdate::errc::invalid_character:
    case nepdate::errc::wrong_group_count:
    case nepdate::errc::number_too_long:
    case nepdate::errc::ambiguous:
    case nepdate::errc::unrecognized:
        return true;
    case nepdate::errc::invalid_gregorian:
        break;
    }
    return false;
}

/// Strict parsing: no crash (ASan and UBSan in the fuzz build), every error is one of the eight
/// kinds, and a success formats with to_chars and parses back to the same date.
[[nodiscard]] inline bool parse_property(std::string_view input) noexcept {
    const nepdate::result<nepdate::date> r = nepdate::parse(input);
    if (!r.has_value()) {
        return is_parse_kind(r.error());
    }
    std::array<char, nepdate::max_long_size> buf{};
    const std::to_chars_result w = nepdate::to_chars(buf.data(), buf.data() + buf.size(), *r);
    return w.ec == std::errc{} &&
           nepdate::parse(
               std::string_view(buf.data(), static_cast<std::size_t>(w.ptr - buf.data()))) == r;
}

/// The same property for parse_lenient.
[[nodiscard]] inline bool parse_lenient_property(std::string_view input) noexcept {
    const nepdate::result<nepdate::date> r = nepdate::parse_lenient(input);
    if (!r.has_value()) {
        return is_parse_kind(r.error());
    }
    std::array<char, nepdate::max_long_size> buf{};
    const std::to_chars_result w = nepdate::to_chars(buf.data(), buf.data() + buf.size(), *r);
    return w.ec == std::errc{} &&
           nepdate::parse(
               std::string_view(buf.data(), static_cast<std::size_t>(w.ptr - buf.data()))) == r;
}

} // namespace nepdate_test::fuzz

#endif // NEPDATE_TESTS_FUZZ_PROPERTY_HPP
