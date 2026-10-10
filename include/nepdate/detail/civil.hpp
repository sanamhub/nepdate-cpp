#ifndef NEPDATE_DETAIL_CIVIL_HPP
#define NEPDATE_DETAIL_CIVIL_HPP

// Proleptic Gregorian dates <-> Unix days with the Neri-Schneider algorithms (C. Neri and
// L. Schneider, "Euclidean affine functions and their application to calendar algorithms",
// Software: Practice and Experience 53(4), 2023). Same results as ALGORITHM section 5 (Hinnant),
// with multiplications and shifts instead of divisions by 400, 100 and 4 (ADR-0002 section 3). Not
// public API.

#include <cstdint>

namespace nepdate::detail {

struct civil {
    int year;
    unsigned month;
    unsigned day;
};

// The shift moves every year of interest to a positive count of 400-year eras, so all arithmetic
// is unsigned 32-bit: s = 82, K = 719468 + 146097 * s, L = 400 * s.
inline constexpr std::uint32_t kCivilShift = 82;
inline constexpr std::uint32_t kCivilK = 719468U + 146097U * kCivilShift;
inline constexpr std::uint32_t kCivilL = 400U * kCivilShift;

// Unix days of a valid Gregorian date. Precondition: year in -32800..1000000 or so; the library
// calls it for 1844..2143 only.
[[nodiscard]] constexpr int days_from_civil(int year, unsigned month, unsigned day) noexcept {
    const std::uint32_t j = month <= 2U ? 1U : 0U; // January and February count as months 13, 14
    const std::uint32_t y = static_cast<std::uint32_t>(year) + kCivilL - j;
    const std::uint32_t m = j != 0U ? month + 12U : month;
    const std::uint32_t d = day - 1U;
    const std::uint32_t c = y / 100U;
    const std::uint32_t y_days = 1461U * y / 4U - c + c / 4U;
    const std::uint32_t m_days = (979U * m - 2919U) / 32U;
    const std::uint32_t n = y_days + m_days + d;
    return static_cast<int>(n) - static_cast<int>(kCivilK);
}

// Gregorian date of a Unix day. Precondition: days >= -kCivilK.
[[nodiscard]] constexpr civil civil_from_days(int days) noexcept {
    const std::uint32_t n = static_cast<std::uint32_t>(days) + kCivilK;
    // Century and day of the century.
    const std::uint32_t n1 = 4U * n + 3U;
    const std::uint32_t c = n1 / 146097U;
    const std::uint32_t nc = n1 % 146097U / 4U;
    // Year of the century and day of the year: one 64-bit product.
    const std::uint32_t n2 = 4U * nc + 3U;
    const std::uint64_t p2 = std::uint64_t{2939745U} * n2;
    const auto z = static_cast<std::uint32_t>(p2 >> 32U);
    const std::uint32_t ny = static_cast<std::uint32_t>(p2) / 2939745U / 4U;
    const std::uint32_t y = 100U * c + z;
    // Month and day, counting from March.
    const std::uint32_t n3 = 2141U * ny + 197913U;
    const std::uint32_t m = n3 >> 16U;
    const std::uint32_t d = (n3 & 0xFFFFU) / 2141U;
    // January and February belong to the next year.
    const std::uint32_t j = ny >= 306U ? 1U : 0U;
    return civil{static_cast<int>(y + j) - static_cast<int>(kCivilL), j != 0U ? m - 12U : m,
                 d + 1U};
}

// Days in a Gregorian month. Precondition: month in 1..12.
[[nodiscard]] constexpr unsigned gregorian_month_length(int year, unsigned month) noexcept {
    if (month == 2U) {
        const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        return leap ? 29U : 28U;
    }
    return month == 4U || month == 6U || month == 9U || month == 11U ? 30U : 31U;
}

} // namespace nepdate::detail

#endif // NEPDATE_DETAIL_CIVIL_HPP
