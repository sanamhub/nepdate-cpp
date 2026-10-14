#ifndef NEPDATE_CODEGEN_PATRO_HPP
#define NEPDATE_CODEGEN_PATRO_HPP

// The patro data of shared/data/patro/*.json and the tables of ADR-0002 section 7. The generator
// computes serials from the calendar JSON with the ALGORITHM section 4 formula; it never includes
// the library.

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nepdate::codegen {

// One day of one year file, in date order.
struct patro_day {
    int year = 0;
    int month = 0;
    int day = 0;
    bool has_tithi = false;
    std::pair<std::string, std::string> tithi; // np, en; set when has_tithi
    bool holiday = false;
    std::vector<std::pair<std::string, std::string>> events; // np, en pairs, file order
};

struct patro_input {
    // Every day that has any metadata, sorted by date (files sorted by year, days by date).
    std::vector<patro_day> days;
};

struct patro_input_result {
    patro_input value;
    std::string error; // empty on success
};

// Parses the patro year files, given as their JSON texts in any order. Checks schema 1 and that
// days parse as valid BS dates inside the calendar; the calendar check happens in build.
[[nodiscard]] patro_input_result parse_patro(const std::vector<std::string>& year_jsons);

// The tables of ADR-0002 section 7.
struct patro_tables {
    std::uint32_t first = 0;                    // serial of 2077-01-01
    std::uint32_t days = 0;                     // 2793, through 2084-08-20
    std::vector<std::uint8_t> day_tithi;        // one per covered day, 0xFF = none
    std::vector<std::uint64_t> holiday_bits;    // bit i set = day i is a public holiday
    std::vector<std::uint16_t> day_event_start; // events of day i: [start[i], start[i + 1])
    std::vector<std::uint16_t> events;          // name table indices, file order per day
    std::vector<std::uint32_t> holidays;        // packed dates, ascending
    std::vector<std::pair<std::string, std::string>> names; // np, en; the 32 tithi names first
    std::string name_blob;                                  // every name, np then en, concatenated
    std::vector<std::uint32_t> name_offsets; // 2 * names + 1 entries, the last is blob size
};

struct patro_tables_result {
    patro_tables value;
    std::string error; // empty on success
};

// Builds the tables. month_start[i] is the serial of the first day of month index i, with the
// total day count as the last entry (as kMonthStart). Fails naming the date when a tithi day is
// missing inside the contiguous tithi range (PATRO section 3).
[[nodiscard]] patro_tables_result build_patro_tables(const patro_input& input,
                                                     const std::vector<std::uint32_t>& month_start);

// The bytes of patro/src/patro_tables.hpp: octal escapes for the strings (ADR-0001 section 6),
// readable Devanagari only in comments, LF line endings, no timestamps. month_start provides the
// BS date of each rendered day block.
[[nodiscard]] std::string render_patro_tables(const patro_tables& t,
                                              const std::vector<std::uint32_t>& month_start);

// The bytes of patro/include/nepdate/patro_config.hpp.
[[nodiscard]] std::string render_patro_config(std::uint64_t calendar_fingerprint);

} // namespace nepdate::codegen

#endif // NEPDATE_CODEGEN_PATRO_HPP
