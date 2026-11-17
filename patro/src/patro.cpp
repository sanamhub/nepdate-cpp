// The patro queries that stay compiled (PATRO section 2). info() and the name accessors are inline
// in patro.hpp over the generated tables (ADR-0007).
#include <nepdate/patro.hpp>

#include <span>

namespace nepdate::patro {

date_range coverage() noexcept {
    return *date_range::make( // NOLINT(bugprone-unchecked-optional-access): first < last here
        nepdate::detail::from_serial(detail::kFirst),
        nepdate::detail::from_serial(detail::kFirst + detail::kDays - 1U));
}

std::span<const date> public_holidays() noexcept {
    return std::span<const date>{detail::kHolidays.data(), detail::kHolidays.size()};
}

} // namespace nepdate::patro
