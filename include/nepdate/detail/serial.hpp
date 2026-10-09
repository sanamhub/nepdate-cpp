#ifndef NEPDATE_DETAIL_SERIAL_HPP
#define NEPDATE_DETAIL_SERIAL_HPP

// BS date <-> serial day (days since BS 1901-01-01), ADR-0002 §3. Not public API.
// <nepdate/date.hpp> includes this file after it defines date.

#include <nepdate/date.hpp>
#include <nepdate/detail/calendar_tables.hpp>

#include <cstdint>

namespace nepdate::detail {

// to_serial(y, m, d) = kMonthStart[(y-1901)*12 + m-1] + d - 1: one load.
constexpr std::uint32_t to_serial(date d) noexcept {
    return kMonthStart[month_index(d)] + (date_access::packed(d) & 0xFFU) - 1U;
}

} // namespace nepdate::detail

#endif // NEPDATE_DETAIL_SERIAL_HPP
