// The patro queries (PATRO section 2). The tables are generated into patro_tables.hpp and have
// internal linkage here, so they exist once, in this translation unit (ADR-0002 section 7).
#include <nepdate/patro.hpp>

#include "patro_tables.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nepdate::patro {
namespace detail {
namespace {

// Name k of the table (0 = np, 1 = en) through the blob and its offsets.
std::string_view name_of(std::uint16_t k, lang l) noexcept {
    const std::size_t i = static_cast<std::size_t>(k) * 2U + (l == lang::ne ? 0U : 1U);
    const std::uint32_t from = kNameOffsets[i];
    const std::uint32_t to = kNameOffsets[i + 1];
    return std::string_view(kNameBlob.data() + from, to - from);
}

} // namespace

} // namespace detail

std::string_view tithi::name(lang l) const noexcept {
    return detail::name_of(static_cast<std::uint16_t>(index_), l);
}

std::string_view event::name(lang l) const noexcept {
    return detail::name_of(index_, l);
}

day_info info(date d) noexcept {
    const std::uint32_t serial = nepdate::detail::to_serial(d);
    const std::uint32_t offset = serial - detail::kFirst;
    if (offset >= detail::kDays) {
        return day_info{detail::tag{}, nullptr, 0, 0xFFU, false};
    }
    const std::uint8_t tithi_index = detail::kDayTithi[offset];
    const std::uint16_t first = detail::kDayEventStart[offset];
    const std::uint16_t count =
        static_cast<std::uint16_t>(detail::kDayEventStart[offset + 1] - first);
    const bool holiday = (detail::kHolidayBits[offset / 64U] >> (offset % 64U)) & 1U;
    return day_info{detail::tag{}, detail::kEvents.data() + first, count, tithi_index, holiday};
}

date_range coverage() noexcept {
    return *date_range::make(nepdate::detail::from_serial(detail::kFirst),
                             nepdate::detail::from_serial(detail::kFirst + detail::kDays - 1U));
}

std::span<const date> public_holidays() noexcept {
    return std::span<const date>{detail::kHolidays.data(), detail::kHolidays.size()};
}

} // namespace nepdate::patro
