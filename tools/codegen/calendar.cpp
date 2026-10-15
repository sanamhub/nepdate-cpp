#include "calendar.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace nepdate::codegen {
namespace {

constexpr int kMinYear = 1901;
constexpr int kMaxYear = 2199;
constexpr std::size_t kYears = kMaxYear - kMinYear + 1;
constexpr std::size_t kMonths = 12;
constexpr std::int64_t kMinMonthLength = 29;
constexpr std::int64_t kMaxMonthLength = 32;
constexpr std::string_view kEpochBs = "1901-01-01";
constexpr std::string_view kEpochAd = "1844-04-11";
constexpr std::uint32_t kBlockSize = 32;
constexpr std::uint32_t kMaxBlockSteps = 2;
constexpr std::size_t kBlockLine = 16;
constexpr std::uint64_t kFnvOffsetBasis = 0xcbf29ce484222325;
constexpr std::uint64_t kFnvPrime = 0x100000001b3;

using json = nlohmann::json;

const json* member(const json& object, const char* key) {
    if (!object.is_object()) {
        return nullptr;
    }
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &*it;
}

bool has_int(const json& object, const char* key, std::int64_t expected) {
    const json* value = member(object, key);
    return value != nullptr && value->is_number_integer() && value->get<std::int64_t>() == expected;
}

bool has_string(const json* object, const char* key, std::string_view expected) {
    const json* value = object == nullptr ? nullptr : member(*object, key);
    return value != nullptr && value->is_string() &&
           value->get_ref<const std::string&>() == expected;
}

// ALGORITHM section 5 (Hinnant) for the one date the header needs: the epoch, 1844-04-11.
int epoch_unix_days() {
    const int y = 1844;
    const int m = 4;
    const int d = 11;
    const int era = y / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * ((m + 9) % 12) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

std::string hex64(std::uint64_t value) {
    constexpr std::string_view digits = "0123456789abcdef";
    std::string out = "0x";
    for (int shift = 60; shift >= 0; shift -= 4) {
        out += digits.at((value >> static_cast<unsigned>(shift)) & 0xfU);
    }
    return out;
}

parse_result fail(std::string message) {
    parse_result result;
    result.error = "bs-calendar.json: " + std::move(message);
    return result;
}

} // namespace

parse_result parse_calendar(std::string_view json_text) {
    // allow_exceptions = false: a syntax error gives a discarded value instead of a throw.
    const json doc = json::parse(json_text, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) {
        return fail("not a JSON object");
    }
    if (!has_int(doc, "min_year", kMinYear)) {
        return fail("min_year must be 1901");
    }
    if (!has_int(doc, "max_year", kMaxYear)) {
        return fail("max_year must be 2199");
    }
    const json* epoch = member(doc, "epoch");
    if (!has_string(epoch, "bs", kEpochBs) || !has_string(epoch, "ad", kEpochAd)) {
        return fail("epoch must be bs 1901-01-01, ad 1844-04-11");
    }
    const json* lengths = member(doc, "month_lengths");
    if (lengths == nullptr || !lengths->is_object() || lengths->size() != kYears) {
        return fail("month_lengths must hold the 299 years 1901 to 2199");
    }

    parse_result result;
    result.value.min_year = kMinYear;
    result.value.max_year = kMaxYear;
    result.value.month_lengths.reserve(kYears);
    for (int year = kMinYear; year <= kMaxYear; ++year) {
        const std::string key = std::to_string(year);
        const json* months = member(*lengths, key.c_str());
        if (months == nullptr || !months->is_array() || months->size() != kMonths) {
            return fail("year " + key + ": expected 12 month lengths");
        }
        std::array<int, kMonths> row{};
        for (std::size_t m = 0; m < kMonths; ++m) {
            const json& value = (*months)[m];
            const std::string where = "year " + key + " month " + std::to_string(m + 1);
            if (!value.is_number_integer()) {
                return fail(where + ": length is not an integer");
            }
            const auto length = value.get<std::int64_t>();
            if (length < kMinMonthLength || length > kMaxMonthLength) {
                return fail(where + ": length " + std::to_string(length) + " is outside 29..32");
            }
            row.at(m) = static_cast<int>(length);
        }
        result.value.month_lengths.push_back(row);
    }
    return result;
}

tables_result build_tables(const calendar& cal) {
    tables_result result;
    tables& t = result.value;
    std::uint32_t serial = 0;
    std::uint64_t hash = kFnvOffsetBasis;
    t.month_start.reserve(cal.month_lengths.size() * kMonths + 1);
    for (const auto& year : cal.month_lengths) {
        for (const int length : year) {
            t.month_start.push_back(serial);
            serial += static_cast<std::uint32_t>(length);
            hash ^= static_cast<std::uint8_t>(length);
            hash *= kFnvPrime;
        }
    }
    t.month_start.push_back(serial);
    t.fingerprint = hash;

    // Block b covers serials 32b..32b+31 and records the month holding 32b. from_serial then takes
    // at most two forward steps (ADR-0002 §3); the last serial of a block needs the most.
    const auto months = static_cast<std::uint32_t>(t.month_start.size() - 1);
    const std::uint32_t blocks = (serial + kBlockSize - 1) / kBlockSize;
    std::uint32_t month = 0;
    for (std::uint32_t b = 0; b < blocks; ++b) {
        const std::uint32_t first = b * kBlockSize;
        while (t.month_start.at(month + 1) <= first) {
            ++month;
        }
        t.block_month.push_back(static_cast<std::uint16_t>(month));
        const std::uint32_t last = std::min(first + kBlockSize - 1, serial - 1);
        std::uint32_t steps = 0;
        for (std::uint32_t i = month; i < months && t.month_start.at(i + 1) <= last; ++i) {
            ++steps;
        }
        if (steps > kMaxBlockSteps) {
            result.error = "block " + std::to_string(b) + " (serials " + std::to_string(first) +
                           ".." + std::to_string(last) + ") needs " + std::to_string(steps) +
                           " steps, at most 2 allowed";
            return result;
        }
    }
    return result;
}

std::string render_calendar_tables(const calendar& cal, const tables& t) {
    std::string out;
    out += "// Generated by tools/codegen from shared/data. DO NOT EDIT.\n";
    out += "// Source: shared/data/calendar/bs-calendar.json. Regenerate with the codegen target\n";
    out += "// (ADR-0001 section 5). Layout and use: ADR-0002 section 2.\n";
    out += "#ifndef NEPDATE_DETAIL_CALENDAR_TABLES_HPP\n";
    out += "#define NEPDATE_DETAIL_CALENDAR_TABLES_HPP\n";
    out += "\n";
    out += "#include <array>\n";
    out += "#include <cstdint>\n";
    out += "\n";
    out += "namespace nepdate::detail {\n";
    out += "\n";
    out += "inline constexpr int kMinYear = " + std::to_string(cal.min_year) + ";\n";
    out += "inline constexpr int kMaxYear = " + std::to_string(cal.max_year) + ";\n";
    out +=
        "inline constexpr std::uint32_t kMaxSerial = " + std::to_string(t.month_start.back() - 1) +
        ";\n";
    out += "// Unix day of the epoch, AD 1844-04-11 = BS 1901-01-01.\n";
    out += "inline constexpr int kEpochUnixDays = " + std::to_string(epoch_unix_days()) + ";\n";
    out += "// FNV-1a 64 of the " + std::to_string(t.month_start.size() - 1) +
           " month lengths, one byte each.\n";
    out += "inline constexpr std::uint64_t kCalendarFingerprint = " + hex64(t.fingerprint) + ";\n";
    out += "\n";
    out +=
        "// Serial of the first day of month index (y - 1901) * 12 + (m - 1). The last entry is\n";
    out += "// the number of days in the calendar.\n";
    out += "inline constexpr std::array<std::uint32_t, " + std::to_string(t.month_start.size()) +
           "> kMonthStart = {\n";
    for (std::size_t y = 0; y < cal.month_lengths.size(); ++y) {
        out += "   ";
        for (std::size_t m = 0; m < kMonths; ++m) {
            out += " " + std::to_string(t.month_start.at(y * kMonths + m)) + ",";
        }
        out += " // " + std::to_string(cal.min_year + static_cast<int>(y)) + "\n";
    }
    out += "    " + std::to_string(t.month_start.back()) + ",\n";
    out += "};\n";
    out += "\n";
    out += "// Month index holding serial 32 * b, for block b.\n";
    out += "inline constexpr std::array<std::uint16_t, " + std::to_string(t.block_month.size()) +
           "> kBlockMonth = {\n";
    for (std::size_t i = 0; i < t.block_month.size(); i += kBlockLine) {
        out += "   ";
        for (std::size_t j = i; j < std::min(i + kBlockLine, t.block_month.size()); ++j) {
            out += " " + std::to_string(t.block_month.at(j)) + ",";
        }
        out += "\n";
    }
    out += "};\n";
    out += "\n";
    out += "} // namespace nepdate::detail\n";
    out += "\n";
    out += "#endif // NEPDATE_DETAIL_CALENDAR_TABLES_HPP\n";
    return out;
}

} // namespace nepdate::codegen
