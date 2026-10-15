#ifndef NEPDATE_TESTS_REFERENCE_PARSE_LENIENT_C6_HPP
#define NEPDATE_TESTS_REFERENCE_PARSE_LENIENT_C6_HPP

// C7-02: the pre-split parse_lenient exactly as it was at the C6 review, kept as the reference the
// new detail::tokenize/detail::assemble implementation must agree with on every input. Test code,
// not part of the library.

#include <nepdate/parse.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace nepdate_test::reference {

// The C6 parse_lenient (PARSING section 2), renamed. The library's parse_lenient must give the
// same value or error for every input.
// NOLINTNEXTLINE(readability-function-cognitive-complexity): reference copy kept verbatim
[[nodiscard]] inline nepdate::result<nepdate::date> parse_lenient_c6(std::string_view s) noexcept {
    if (const nepdate::result<nepdate::date> r = nepdate::parse(s); r.has_value()) {
        return r;
    }

    std::array<int, 3> nums{};
    std::array<int, 3> num_digits{};
    std::size_t num_count = 0;
    int month_value = 0;
    std::size_t month_count = 0;
    std::size_t unknown_count = 0;

    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t sep = nepdate::detail::lenient_sep_size(s, i);
        if (sep != 0) {
            i += sep;
            continue;
        }
        const std::size_t start = i;
        while (i < s.size() && nepdate::detail::lenient_sep_size(s, i) == 0) {
            ++i;
        }
        const std::string_view token = s.substr(start, i - start);
        if (nepdate::detail::is_digits_and_dots(token)) {
            std::size_t p = 0;
            while (p <= token.size()) {
                const std::size_t dot = token.find('.', p);
                const std::size_t end = dot == std::string_view::npos ? token.size() : dot;
                if (end > p) {
                    int digits = 0;
                    int value = 0;
                    nepdate::detail::read_digits(token.substr(p, end - p), digits, value);
                    if (digits > 4) {
                        return nepdate::errc::number_too_long;
                    }
                    if (num_count < nums.size()) {
                        nums[num_count] = value;
                        num_digits[num_count] = digits;
                    }
                    ++num_count;
                }
                if (dot == std::string_view::npos) {
                    break;
                }
                p = end + 1;
            }
        } else {
            bool dropped = false;
            for (const std::string_view word : nepdate::detail::kDroppedWords) {
                if (token == word) {
                    dropped = true;
                    break;
                }
            }
            if (!dropped) {
                for (const std::string_view era : nepdate::detail::kEraMarkers) {
                    if (nepdate::detail::folded_eq(token, era)) {
                        dropped = true;
                        break;
                    }
                }
            }
            if (dropped) {
                continue;
            }
            int m = 0;
            for (const nepdate::detail::month_spelling& spelling :
                 nepdate::detail::kMonthSpellings) {
                if (nepdate::detail::folded_eq(token, spelling.text)) {
                    m = static_cast<int>(spelling.month);
                    break;
                }
            }
            if (m != 0) {
                if (month_count == 0) {
                    month_value = m;
                }
                ++month_count;
            } else {
                ++unknown_count;
            }
        }
    }

    if (month_count == 1 && unknown_count == 0 && num_count == 2) {
        int year = 0;
        int day = 0;
        int day_digits = 0;
        if (num_digits[0] == 4 && num_digits[1] != 4) {
            year = nums[0];
            day = nums[1];
            day_digits = num_digits[1];
        } else if (num_digits[1] == 4 && num_digits[0] != 4) {
            year = nums[1];
            day = nums[0];
            day_digits = num_digits[0];
        } else {
            return nepdate::errc::ambiguous;
        }
        if (day_digits > 2) {
            return nepdate::errc::ambiguous;
        }
        return nepdate::make_date(year, month_value, day);
    }
    if (month_count == 0 && unknown_count == 0 && num_count == 3) {
        if (num_digits[0] == 4) {
            return nepdate::make_date(nums[0], nums[1], nums[2]);
        }
        if (num_digits[2] == 4) {
            return nepdate::make_date(nums[2], nums[1], nums[0]);
        }
        return nepdate::errc::ambiguous;
    }
    return nepdate::errc::unrecognized;
}

} // namespace nepdate_test::reference

#endif // NEPDATE_TESTS_REFERENCE_PARSE_LENIENT_C6_HPP
