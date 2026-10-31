#include "patro.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nepdate::codegen {
namespace {

using json = nlohmann::json;

constexpr int kMinYear = 1901;
constexpr std::size_t kMonthsPerYear = 12;
constexpr std::uint32_t kNoTithi = 0xFFU;
constexpr std::size_t kTithiPerLine = 32;
constexpr std::size_t kEventPerLine = 16;
constexpr std::size_t kHolidayBitsPerLine = 4;

const json* member(const json& object, const char* key) {
    if (!object.is_object()) {
        return nullptr;
    }
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &*it;
}

std::string fail(const std::string& message) {
    return "patro data: " + message;
}

// "2081-01-01" to its parts; false when the key doesn't parse.
bool parse_date_key(std::string_view key, int& year, int& month, int& day) {
    if (key.size() != 10 || key[4] != '-' || key[7] != '-') {
        return false;
    }
    const auto digits = [](std::string_view s) -> int {
        int value = 0;
        for (const char c : s) {
            if (c < '0' || c > '9') {
                return -1;
            }
            value = value * 10 + (c - '0');
        }
        return value;
    };
    year = digits(key.substr(0, 4));
    month = digits(key.substr(5, 2));
    day = digits(key.substr(8, 2));
    return year > 0 && month > 0 && day > 0;
}

// A name pair of PATRO section 1: both strings present and non-empty.
bool name_pair(const json* object, std::pair<std::string, std::string>& out) {
    if (object == nullptr || !object->is_object()) {
        return false;
    }
    const json* np = member(*object, "np");
    const json* en = member(*object, "en");
    if (np == nullptr || en == nullptr || !np->is_string() || !en->is_string() ||
        np->get_ref<const std::string&>().empty() || en->get_ref<const std::string&>().empty()) {
        return false;
    }
    out = {np->get_ref<const std::string&>(), en->get_ref<const std::string&>()};
    return true;
}

std::string date_text(int year, int month, int day) {
    const auto pad = [](int value) {
        const std::string s = std::to_string(value);
        return s.size() < 2 ? "0" + s : s;
    };
    return std::to_string(year) + "-" + pad(month) + "-" + pad(day);
}

// "YYYY-MM-DD" of a serial inside the calendar. month_start has kMonths + 1 entries, the last
// being the total day count.
std::string date_of_serial(const std::vector<std::uint32_t>& month_start, std::uint32_t serial) {
    std::size_t i = 0;
    while (month_start[i + 1] <= serial) {
        ++i;
    }
    const int year = kMinYear + static_cast<int>(i / kMonthsPerYear);
    const int month = static_cast<int>(i % kMonthsPerYear) + 1;
    const int day = static_cast<int>(serial - month_start[i]) + 1;
    return date_text(year, month, day);
}

int year_of_serial(const std::vector<std::uint32_t>& month_start, std::uint32_t serial) {
    std::size_t i = 0;
    while (month_start[i + 1] <= serial) {
        ++i;
    }
    return kMinYear + static_cast<int>(i / kMonthsPerYear);
}

// The flat summary has one object of integers and date strings; these three read one field and
// name it when it is missing or has the wrong type.
bool summary_integer(const json& doc, const char* key, std::int64_t& out, std::string& error) {
    const json* value = member(doc, key);
    if (value == nullptr) {
        error = fail("summary field '" + std::string(key) + "' is missing");
        return false;
    }
    if (!value->is_number_integer()) {
        error = fail("summary field '" + std::string(key) + "' must be an integer");
        return false;
    }
    out = value->get<std::int64_t>();
    return true;
}

bool summary_text(const json& doc, const char* key, std::string& out, std::string& error) {
    const json* value = member(doc, key);
    if (value == nullptr) {
        error = fail("summary field '" + std::string(key) + "' is missing");
        return false;
    }
    if (!value->is_string()) {
        error = fail("summary field '" + std::string(key) + "' must be a string");
        return false;
    }
    out = value->get<std::string>();
    return true;
}

std::string hex(std::uint64_t value, int digits) {
    constexpr std::string_view hex_digits = "0123456789abcdef";
    std::string out = "0x";
    for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4) {
        out += hex_digits.at((value >> static_cast<unsigned>(shift)) & 0xfU);
    }
    return out;
}

// One byte as a char literal, so the blob array holds exactly its bytes and no string terminators.
std::string char_literal(unsigned char byte) {
    if (byte >= 0x20U && byte < 0x7FU && byte != '\'' && byte != '\\') {
        return std::string("'") + static_cast<char>(byte) + "'";
    }
    return "'\\" + std::to_string(byte / 64U) + std::to_string(byte / 8U % 8U) +
           std::to_string(byte % 8U) + "'";
}

// One name's bytes as char literals, 16 per line.
void blob_bytes(std::string& out, std::string_view text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i % 16U == 0U) {
            out += "    ";
        }
        out += char_literal(static_cast<unsigned char>(text[i])) + ",";
        if (i % 16U == 15U) {
            out += "\n";
        }
    }
    if (text.size() % 16U != 0U) {
        out += "\n";
    }
}

} // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one validation per JSON field, build-time tool
patro_input_result parse_patro(const std::vector<std::string>& year_jsons) {
    patro_input_result result;
    for (const std::string& text : year_jsons) {
        const json doc = json::parse(text, nullptr, false);
        if (doc.is_discarded() || !doc.is_object()) {
            result.error = fail("a year file is not a JSON object");
            return result;
        }
        const json* schema = member(doc, "schema");
        if (schema == nullptr || !schema->is_number_integer() || schema->get<std::int64_t>() != 1) {
            result.error = fail("schema must be 1");
            return result;
        }
        const json* year = member(doc, "year");
        if (year == nullptr || !year->is_number_integer()) {
            result.error = fail("year must be an integer");
            return result;
        }
        const int year_number = static_cast<int>(year->get<std::int64_t>());
        const json* days = member(doc, "days");
        if (days == nullptr || !days->is_object() || days->empty()) {
            result.error = fail("year " + std::to_string(year_number) +
                                ": days must be a "
                                "non-empty object");
            return result;
        }
        // nlohmann-json keeps object keys sorted, so days iterate in date order.
        for (const auto& [key, value] : days->items()) {
            patro_day entry;
            entry.year = year_number;
            int key_year = 0;
            if (!parse_date_key(key, key_year, entry.month, entry.day) || key_year != year_number) {
                result.error = fail("day key '" + key + "' does not match year " +
                                    std::to_string(year_number));
                return result;
            }
            if (!value.is_object()) {
                result.error = fail(key + ": day must be an object");
                return result;
            }
            if (member(value, "tithi") != nullptr) {
                if (!name_pair(member(value, "tithi"), entry.tithi)) {
                    result.error = fail(key + ": tithi must have non-empty np and en");
                    return result;
                }
                entry.has_tithi = true;
            }
            const json* holiday = member(value, "holiday");
            if (holiday != nullptr) {
                if (!holiday->is_boolean()) {
                    result.error = fail(key + ": holiday must be a boolean");
                    return result;
                }
                entry.holiday = holiday->get<bool>();
            }
            const json* events = member(value, "events");
            if (events != nullptr) {
                if (!events->is_array()) {
                    result.error = fail(key + ": events must be an array");
                    return result;
                }
                for (const json& event : *events) {
                    std::pair<std::string, std::string> pair;
                    if (!name_pair(&event, pair)) {
                        result.error = fail(key + ": an event lacks non-empty np and en");
                        return result;
                    }
                    entry.events.push_back(std::move(pair));
                }
            }
            result.value.days.push_back(std::move(entry));
        }
    }
    // Files arrive in any order; the tables need date order.
    std::sort(result.value.days.begin(), result.value.days.end(),
              [](const patro_day& a, const patro_day& b) {
                  if (a.year != b.year) {
                      return a.year < b.year;
                  }
                  if (a.month != b.month) {
                      return a.month < b.month;
                  }
                  return a.day < b.day;
              });
    return result;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one check per summary field, build-time tool
patro_summary_result parse_patro_summary(std::string_view text) {
    patro_summary_result result;
    const json doc = json::parse(text, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) {
        result.error = fail("the patro summary is not a JSON object");
        return result;
    }
    const auto count = [&](const char* key, std::uint32_t& out) {
        std::int64_t value = 0;
        if (!summary_integer(doc, key, value, result.error)) {
            return false;
        }
        if (value < 0 || value > 0xFFFFFFFFLL) {
            result.error = fail("summary field '" + std::string(key) + "' is out of range");
            return false;
        }
        out = static_cast<std::uint32_t>(value);
        return true;
    };
    const auto year = [&](const char* key, int& out) {
        std::int64_t value = 0;
        if (!summary_integer(doc, key, value, result.error)) {
            return false;
        }
        if (value < 1 || value > 9999) {
            result.error = fail("summary field '" + std::string(key) + "' is out of range");
            return false;
        }
        out = static_cast<int>(value);
        return true;
    };
    const auto date = [&](const char* key, std::string& out) {
        return summary_text(doc, key, out, result.error);
    };

    std::int64_t schema = 0;
    if (!summary_integer(doc, "schema", schema, result.error)) {
        return result;
    }
    if (schema != 1) {
        result.error = fail("summary schema must be 1, got " + std::to_string(schema));
        return result;
    }
    result.value.schema = static_cast<int>(schema);
    patro_summary& s = result.value;
    if (!year("first_year", s.first_year) || !year("last_year", s.last_year) ||
        !date("first_day", s.first_day) || !date("last_day", s.last_day) ||
        !count("first_serial", s.first_serial) || !count("last_serial", s.last_serial) ||
        !count("days", s.days) || !date("tithi_first_day", s.tithi_first_day) ||
        !date("tithi_last_day", s.tithi_last_day) ||
        !count("tithi_first_serial", s.tithi_first_serial) || !count("tithi_days", s.tithi_days) ||
        !count("tithi_names", s.tithi_names) || !count("holidays", s.holidays) ||
        !count("event_days", s.event_days) || !count("events", s.events) ||
        !count("event_names", s.event_names) || !count("names", s.names) ||
        !count("name_bytes", s.name_bytes)) {
        return result;
    }
    return result;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one pass per table, build-time tool
patro_tables_result build_patro_tables(const patro_input& input,
                                       const std::vector<std::uint32_t>& month_start) {
    patro_tables_result result;
    if (input.days.empty()) {
        result.error = fail("no days with metadata");
        return result;
    }
    const auto months = month_start.size() - 1;
    const auto serial_of = [&](const patro_day& d) -> std::uint32_t {
        const auto y = static_cast<std::uint32_t>(d.year) - static_cast<std::uint32_t>(kMinYear);
        const auto i = y * kMonthsPerYear + static_cast<std::uint32_t>(d.month) - 1U;
        if (i >= months) {
            return 0xFFFFFFFFU;
        }
        return month_start[i] + static_cast<std::uint32_t>(d.day) - 1U;
    };
    const auto valid = [&](const patro_day& d) -> bool {
        const auto y = static_cast<std::uint32_t>(d.year) - static_cast<std::uint32_t>(kMinYear);
        const auto i = y * kMonthsPerYear + static_cast<std::uint32_t>(d.month) - 1U;
        return i < months && d.day >= 1 &&
               static_cast<std::uint32_t>(d.day) <= month_start[i + 1] - month_start[i];
    };
    for (const patro_day& d : input.days) {
        if (!valid(d)) {
            result.error = fail(date_text(d.year, d.month, d.day) + " is not a BS date in "
                                                                    "1901..2199");
            return result;
        }
    }

    // Names in first-seen order: the tithi names first, then the event names (ADR-0002 section 7).
    patro_tables& t = result.value;
    std::map<std::pair<std::string, std::string>, std::uint16_t> name_index;
    const auto intern = [&](const std::pair<std::string, std::string>& pair) -> std::uint16_t {
        const auto [it, inserted] =
            name_index.emplace(pair, static_cast<std::uint16_t>(t.names.size()));
        if (inserted) {
            t.names.push_back(pair);
        }
        return it->second;
    };
    std::uint32_t tithi_first = 0;
    std::uint32_t tithi_last = 0;
    bool any_tithi = false;
    for (const patro_day& d : input.days) {
        if (!d.has_tithi) {
            continue;
        }
        intern(d.tithi);
        // A day stores its tithi as one byte, so the tithi names must fit in 256 entries.
        if (t.names.size() > 0x100U) {
            result.error = fail("more than 256 tithi names");
            return result;
        }
        const std::uint32_t serial = serial_of(d);
        if (!any_tithi) {
            tithi_first = serial;
            any_tithi = true;
        }
        tithi_last = serial;
    }
    if (!any_tithi) {
        result.error = fail("no tithi days");
        return result;
    }
    for (const patro_day& d : input.days) {
        for (const auto& pair : d.events) {
            intern(pair);
        }
    }
    if (t.names.size() > 0xFFFFU) {
        result.error = fail("more than 65535 distinct names");
        return result;
    }

    t.first = serial_of(input.days.front());
    t.days = serial_of(input.days.back()) - t.first + 1;
    if (t.days > 0xFFFFU) {
        result.error = fail("coverage longer than uint16 offsets allow");
        return result;
    }
    // The coverage bounds come from the data; validate_patro_tables checks them against the hub
    // summary.

    // One dense walk over the covered days (ADR-0002 section 7).
    t.day_tithi.assign(t.days, static_cast<std::uint8_t>(kNoTithi));
    t.holiday_bits.assign((static_cast<std::size_t>(t.days) + 63U) / 64U, 0);
    t.day_event_start.assign(static_cast<std::size_t>(t.days) + 1, 0);
    std::uint16_t events_seen = 0;
    std::size_t day_index = 0;
    for (std::uint32_t offset = 0; offset < t.days; ++offset) {
        t.day_event_start[offset] = events_seen;
        while (day_index < input.days.size() &&
               serial_of(input.days[day_index]) - t.first == offset) {
            const patro_day& d = input.days[day_index];
            if (d.has_tithi) {
                t.day_tithi[offset] = static_cast<std::uint8_t>(name_index.find(d.tithi)->second);
            }
            if (d.holiday) {
                t.holidays.push_back((static_cast<std::uint32_t>(d.year) << 16U) |
                                     (static_cast<std::uint32_t>(d.month) << 8U) |
                                     static_cast<std::uint32_t>(d.day));
                t.holiday_bits[offset / 64U] |= 1ULL << (offset % 64U);
            }
            for (const auto& pair : d.events) {
                if (events_seen == 0xFFFFU) {
                    result.error = fail("more than 65535 events");
                    return result;
                }
                t.events.push_back(name_index.find(pair)->second);
                ++events_seen;
            }
            ++day_index;
        }
    }
    t.day_event_start[t.days] = events_seen;

    // PATRO section 3: tithi data is contiguous, one entry per day.
    for (std::uint32_t serial = tithi_first; serial <= tithi_last; ++serial) {
        if (t.day_tithi[serial - t.first] == kNoTithi) {
            result.error = fail("tithi gap at " + date_of_serial(month_start, serial));
            return result;
        }
    }

    t.name_offsets.reserve(t.names.size() * 2 + 1);
    for (const auto& [np, en] : t.names) {
        t.name_offsets.push_back(static_cast<std::uint32_t>(t.name_blob.size()));
        t.name_blob += np;
        t.name_offsets.push_back(static_cast<std::uint32_t>(t.name_blob.size()));
        t.name_blob += en;
    }
    t.name_offsets.push_back(static_cast<std::uint32_t>(t.name_blob.size()));
    return result;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one check per summary field, build-time tool
std::string validate_patro_tables(const patro_tables& t, const patro_summary& summary,
                                  const std::vector<std::uint32_t>& month_start) {
    std::string error;
    const auto add = [&error](const char* field, const std::string& actual,
                              const std::string& expected) {
        if (!error.empty()) {
            error += "; ";
        }
        error += fail(std::string(field) + ": data has " + actual + ", summary says " + expected);
    };
    const auto check = [&add](const char* field, std::uint64_t actual, std::uint64_t expected) {
        if (actual != expected) {
            add(field, std::to_string(actual), std::to_string(expected));
        }
    };
    const auto check_text = [&add](const char* field, const std::string& actual,
                                   const std::string& expected) {
        if (actual != expected) {
            add(field, actual, expected);
        }
    };

    if (t.days == 0) {
        add("days", "0", std::to_string(summary.days));
        return error;
    }
    const std::uint32_t last_serial = t.first + t.days - 1U;
    check("first_serial", t.first, summary.first_serial);
    check("last_serial", last_serial, summary.last_serial);
    check("days", t.days, summary.days);
    const std::string first_day = date_of_serial(month_start, t.first);
    const std::string last_day = date_of_serial(month_start, last_serial);
    check_text("first_day", first_day, summary.first_day);
    check_text("last_day", last_day, summary.last_day);
    check("first_year", static_cast<std::uint64_t>(year_of_serial(month_start, t.first)),
          static_cast<std::uint64_t>(summary.first_year));
    check("last_year", static_cast<std::uint64_t>(year_of_serial(month_start, last_serial)),
          static_cast<std::uint64_t>(summary.last_year));

    std::uint32_t tithi_days = 0;
    std::uint32_t tithi_first_serial = 0;
    std::uint32_t tithi_last_serial = 0;
    std::uint32_t tithi_names = 0;
    bool any_tithi = false;
    std::array<bool, 0x100U> seen{};
    for (std::size_t offset = 0; offset < t.day_tithi.size(); ++offset) {
        const std::uint8_t index = t.day_tithi[offset];
        if (index == kNoTithi) {
            continue;
        }
        ++tithi_days;
        if (!any_tithi) {
            tithi_first_serial = t.first + static_cast<std::uint32_t>(offset);
            any_tithi = true;
        }
        tithi_last_serial = t.first + static_cast<std::uint32_t>(offset);
        if (!seen.at(index)) {
            seen.at(index) = true;
            ++tithi_names;
        }
    }
    check("tithi_days", tithi_days, summary.tithi_days);
    check("tithi_names", tithi_names, summary.tithi_names);
    if (any_tithi) {
        check("tithi_first_serial", tithi_first_serial, summary.tithi_first_serial);
        // The summary states the tithi span as a first serial and a day count; the last serial
        // follows from them, since the span must be contiguous.
        check("tithi_last_serial", tithi_last_serial,
              static_cast<std::uint64_t>(summary.tithi_first_serial) + summary.tithi_days - 1U);
        check_text("tithi_first_day", date_of_serial(month_start, tithi_first_serial),
                   summary.tithi_first_day);
        check_text("tithi_last_day", date_of_serial(month_start, tithi_last_serial),
                   summary.tithi_last_day);
    } else {
        check_text("tithi_first_day", "", summary.tithi_first_day);
        check_text("tithi_last_day", "", summary.tithi_last_day);
    }

    std::uint32_t event_days = 0;
    for (std::size_t i = 0; i + 1 < t.day_event_start.size(); ++i) {
        event_days += t.day_event_start[i] < t.day_event_start[i + 1] ? 1U : 0U;
    }
    check("event_days", event_days, summary.event_days);
    check("events", t.events.size(), summary.events);
    check("holidays", t.holidays.size(), summary.holidays);
    check("names", t.names.size(), summary.names);
    check("name_bytes", t.name_blob.size(), summary.name_bytes);
    // Distinct event names, whether or not an event shares a pair with a tithi name.
    std::vector<std::uint8_t> event_name_seen(t.names.size(), 0);
    std::uint32_t event_names = 0;
    for (const std::uint16_t index : t.events) {
        if (event_name_seen[index] == 0) {
            event_name_seen[index] = 1;
            ++event_names;
        }
    }
    check("event_names", event_names, summary.event_names);
    return error;
}

std::string render_patro_tables(const patro_tables& t,
                                const std::vector<std::uint32_t>& month_start) {
    std::string out;
    out += "// Generated by tools/codegen from shared/data. DO NOT EDIT.\n";
    out += "// Source: shared/data/patro/*.json. Regenerate with the codegen target (ADR-0001\n";
    out += "// section 5). Layout and use: ADR-0002 section 7. This private header is included\n";
    out += "// only by patro.cpp and is never installed.\n";
    out += "#ifndef NEPDATE_PATRO_TABLES_HPP\n";
    out += "#define NEPDATE_PATRO_TABLES_HPP\n";
    out += "\n";
    out += "#include <nepdate/patro.hpp>\n";
    out += "\n";
    out += "#include <array>\n";
    out += "#include <cstdint>\n";
    out += "\n";
    out += "namespace nepdate::patro::detail {\n";
    out += "\n";
    out += "// Serial of the first covered day, 2077-01-01.\n";
    out += "inline constexpr std::uint32_t kFirst = " + std::to_string(t.first) + ";\n";
    out += "// Covered days: 2077-01-01 through 2084-08-20.\n";
    out += "inline constexpr std::uint32_t kDays = " + std::to_string(t.days) + ";\n";
    out += "\n";
    out += "// Tithi name index of each covered day, 0xFF = none.\n";
    out += "inline constexpr std::array<std::uint8_t, " + std::to_string(t.day_tithi.size()) +
           "> kDayTithi = {\n";
    for (std::size_t i = 0; i < t.day_tithi.size(); i += kTithiPerLine) {
        out += "   ";
        for (std::size_t j = i; j < std::min(i + kTithiPerLine, t.day_tithi.size()); ++j) {
            out += " " + hex(t.day_tithi[j], 2) + ",";
        }
        out += " // " + date_of_serial(month_start, t.first + static_cast<std::uint32_t>(i)) + "\n";
    }
    out += "};\n";
    out += "\n";
    out += "// Bit i set = day i is a public holiday.\n";
    out += "inline constexpr std::array<std::uint64_t, " + std::to_string(t.holiday_bits.size()) +
           "> kHolidayBits = {\n";
    for (std::size_t i = 0; i < t.holiday_bits.size(); i += kHolidayBitsPerLine) {
        out += "   ";
        for (std::size_t j = i; j < std::min(i + kHolidayBitsPerLine, t.holiday_bits.size()); ++j) {
            out += " " + hex(t.holiday_bits[j], 16) + ",";
        }
        out += " // days " + std::to_string(i * 64U) + ".." +
               std::to_string(std::min(i * 64U + 63U, static_cast<std::size_t>(t.days) - 1U)) +
               "\n";
    }
    out += "};\n";
    out += "\n";
    out += "// Events of day i are kEvents[start[i] .. start[i + 1]), in file order.\n";
    out += "inline constexpr std::array<std::uint16_t, " +
           std::to_string(t.day_event_start.size()) + "> kDayEventStart = {\n";
    for (std::size_t i = 0; i < t.day_event_start.size(); i += kEventPerLine) {
        out += "   ";
        for (std::size_t j = i; j < std::min(i + kEventPerLine, t.day_event_start.size()); ++j) {
            out += " " + std::to_string(t.day_event_start[j]) + ",";
        }
        out += " // " + date_of_serial(month_start, t.first + static_cast<std::uint32_t>(i)) + "\n";
    }
    out += "};\n";
    out += "\n";
    out += "// Name table indices, one per event.\n";
    out += "inline constexpr std::array<nepdate::patro::event, " + std::to_string(t.events.size()) +
           "> kEvents = {{\n";
    for (std::size_t i = 0; i < t.events.size(); i += kEventPerLine) {
        out += "   ";
        for (std::size_t j = i; j < std::min(i + kEventPerLine, t.events.size()); ++j) {
            out += " {nepdate::patro::detail::tag{}, " + std::to_string(t.events[j]) + "},";
        }
        out += "\n";
    }
    out += "}};\n";
    out += "\n";
    out += "// The holiday dates, ascending, as packed dates (year << 16 | month << 8 | day).\n";
    out += "inline constexpr std::array<nepdate::date, " + std::to_string(t.holidays.size()) +
           "> kHolidays = {{\n";
    for (const std::uint32_t packed : t.holidays) {
        out += "    nepdate::detail::date_access::make(" + hex(packed, 8) + "), // " +
               std::to_string(packed >> 16U) + "-" + std::to_string((packed >> 8U) & 0xFFU) + "-" +
               std::to_string(packed & 0xFFU) + "\n";
    }
    out += "}};\n";
    out += "\n";
    out += "// Every distinct name, np then en per pair, concatenated. The 32 tithi names come\n";
    out += "// first (PATRO section 3). The readable Nepali sits beside each pair.\n";
    out += "inline constexpr std::array<char, " + std::to_string(t.name_blob.size()) +
           "> kNameBlob = {\n";
    for (const auto& [np, en] : t.names) {
        out += "    // ";
        out += np;
        out += " (";
        out += en;
        out += ")\n";
        blob_bytes(out, np);
        blob_bytes(out, en);
    }
    out += "};\n";
    out += "\n";
    out += "// Name k in language l (0 = np, 1 = en) is blob[off[2k + l] .. off[2k + l + 1]).\n";
    out += "inline constexpr std::array<std::uint32_t, " + std::to_string(t.name_offsets.size()) +
           "> kNameOffsets = {\n";
    std::size_t offset_index = 0;
    for (std::size_t k = 0; k < t.names.size(); ++k) {
        out += "    " + std::to_string(t.name_offsets[offset_index]) + ", " +
               std::to_string(t.name_offsets[offset_index + 1]) + ", // " + std::to_string(k) +
               ": " + t.names[k].second + " / " + t.names[k].first + "\n";
        offset_index += 2;
    }
    out += "    " + std::to_string(t.name_offsets.back()) + ",\n";
    out += "};\n";
    out += "\n";
    out += "} // namespace nepdate::patro::detail\n";
    out += "\n";
    out += "#endif // NEPDATE_PATRO_TABLES_HPP\n";
    return out;
}

std::string render_patro_config(std::uint64_t calendar_fingerprint) {
    std::string out;
    out += "// Generated by tools/codegen from shared/data. DO NOT EDIT.\n";
    out +=
        "// The fingerprint of the calendar the patro tables were built from; patro.hpp asserts\n";
    out += "// it equals nepdate::detail::kCalendarFingerprint, so a core whose month lengths\n";
    out += "// differ fails to compile instead of returning the wrong day's data (ADR-0002\n";
    out += "// section 7).\n";
    out += "#ifndef NEPDATE_PATRO_CONFIG_HPP\n";
    out += "#define NEPDATE_PATRO_CONFIG_HPP\n";
    out += "\n";
    out += "// Kept overridable so a compile-fail test can inject a wrong value.\n";
    out += "#ifndef NEPDATE_PATRO_CALENDAR_FINGERPRINT\n";
    out += "#define NEPDATE_PATRO_CALENDAR_FINGERPRINT " + hex(calendar_fingerprint, 16) + "\n";
    out += "#endif\n";
    out += "\n";
    out += "#endif // NEPDATE_PATRO_CONFIG_HPP\n";
    return out;
}

} // namespace nepdate::codegen
