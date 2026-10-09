#ifndef NEPDATE_CODEGEN_CALENDAR_HPP
#define NEPDATE_CODEGEN_CALENDAR_HPP

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace nepdate::codegen {

struct calendar {
    int min_year = 0;
    int max_year = 0;
    // month_lengths[y - min_year][m - 1] is the length of BS month m of year y.
    std::vector<std::array<int, 12>> month_lengths;
};

struct parse_result {
    calendar value;
    std::string error; // empty on success
};

// Parses shared/data/calendar/bs-calendar.json and checks it against ADR-0001 §5: years 1901 to
// 2199, 12 lengths of 29 to 32 days each, epoch 1901-01-01 = 1844-04-11. A bad length names its
// year and month, so a data error points at the line to fix in the hub.
[[nodiscard]] parse_result parse_calendar(std::string_view json_text);

// The bytes of include/nepdate/detail/calendar_tables.hpp: LF line endings, no timestamps.
[[nodiscard]] std::string render_calendar_tables(const calendar& cal);

} // namespace nepdate::codegen

#endif // NEPDATE_CODEGEN_CALENDAR_HPP
