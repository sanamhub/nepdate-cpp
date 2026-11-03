#ifndef NEPDATE_CONVERT_HPP
#define NEPDATE_CONVERT_HPP

// Ordered batch conversion between BS and Gregorian dates (NC04 to NC08, D-11). The output is in
// input order and one error fails the whole call.

#include <nepdate/date.hpp>
#include <nepdate/parse.hpp>

#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace nepdate {

/// Returns the BS date of every Gregorian day, in input order, or the first out_of_range.
[[nodiscard]] inline result<std::vector<date>>
to_nepali_dates(std::span<const std::chrono::sys_days> gregorian) {
    std::vector<date> out;
    out.reserve(gregorian.size());
    for (const std::chrono::sys_days day : gregorian) {
        const result<date> converted = from_sys_days(day);
        if (!converted) {
            return converted.error();
        }
        out.push_back(*converted);
    }
    return out;
}

/// Returns the Gregorian day of every strict parse of the strings, in input order, or the first
/// parse or validation error. One bad string fails the whole call.
[[nodiscard]] inline result<std::vector<std::chrono::sys_days>>
to_english_dates(std::span<const std::string> nepali_dates) {
    std::vector<std::chrono::sys_days> out;
    out.reserve(nepali_dates.size());
    for (const std::string& text : nepali_dates) {
        const result<date> parsed = parse(text);
        if (!parsed) {
            return parsed.error();
        }
        out.push_back(parsed->to_sys_days());
    }
    return out;
}

/// Returns the Gregorian day of every date, in input order.
[[nodiscard]] inline std::vector<std::chrono::sys_days>
to_english_dates(std::span<const date> nepali_dates) {
    std::vector<std::chrono::sys_days> out;
    out.reserve(nepali_dates.size());
    for (const date d : nepali_dates) {
        out.push_back(d.to_sys_days());
    }
    return out;
}

} // namespace nepdate

#endif // NEPDATE_CONVERT_HPP
