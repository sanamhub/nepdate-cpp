#ifndef NEPDATE_DETAIL_SERIAL_HPP
#define NEPDATE_DETAIL_SERIAL_HPP

// BS date <-> serial day (days since BS 1901-01-01), ADR-0002 section 3. Not public API.
// <nepdate/date.hpp> includes this file after it defines date.

// NOLINTNEXTLINE(misc-header-include-cycle): split-header pair, see date.hpp
#include <nepdate/date.hpp>
#include <nepdate/detail/calendar_tables.hpp>

#include <cstdint>

namespace nepdate::detail {

// to_serial(y, m, d) = kMonthStart[(y-1901)*12 + m-1] + d - 1: one load.
constexpr std::uint32_t to_serial(date d) noexcept {
    return kMonthStart[month_index(d)] + (date_access::packed(d) & 0xFFU) - 1U;
}

// from_serial(s): i = kBlockMonth[s >> 5], then at most two forward steps (the generator checks
// that bound), written as additions of a comparison so they compile without branches.
// Precondition: s <= kMaxSerial.
constexpr date from_serial(std::uint32_t s) noexcept {
    std::uint32_t i = kBlockMonth[s >> 5U];
    i += static_cast<std::uint32_t>(kMonthStart[i + 1] <= s);
    i += static_cast<std::uint32_t>(kMonthStart[i + 1] <= s);
    const std::uint32_t year = static_cast<std::uint32_t>(kMinYear) + i / 12U;
    const std::uint32_t month = i % 12U + 1U;
    const std::uint32_t day = s - kMonthStart[i] + 1U;
    return date_access::make(pack(year, month, day));
}

} // namespace nepdate::detail

#endif // NEPDATE_DETAIL_SERIAL_HPP
