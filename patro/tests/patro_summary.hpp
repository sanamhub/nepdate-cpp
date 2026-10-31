#ifndef NEPDATE_TESTS_PATRO_SUMMARY_HPP
#define NEPDATE_TESTS_PATRO_SUMMARY_HPP

// A minimal reader for shared/spec/vectors/patro-summary.json, a flat object of integers and ISO
// date strings. The patro library never reads JSON, so the tests parse the summary themselves
// instead of making a JSON library a dependency of the package.

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace nepdate::test {

// The fields of the summary the patro tests compare against the generated tables.
struct patro_summary {
    int schema = 0;
    int first_year = 0;
    int last_year = 0;
    std::string first_day;
    std::string last_day;
    std::uint32_t first_serial = 0;
    std::uint32_t last_serial = 0;
    std::uint32_t days = 0;
    std::string tithi_first_day;
    std::string tithi_last_day;
    std::uint32_t tithi_first_serial = 0;
    std::uint32_t tithi_days = 0;
    std::uint32_t tithi_names = 0;
    std::uint32_t holidays = 0;
    std::uint32_t event_days = 0;
    std::uint32_t events = 0;
    std::uint32_t event_names = 0;
    std::uint32_t names = 0;
    std::uint32_t name_bytes = 0;
};

namespace detail {

inline constexpr std::array<std::string_view, 19> kSummaryKeys = {
    "schema",          "first_year",     "last_year",          "first_day",
    "last_day",        "first_serial",   "last_serial",        "days",
    "tithi_first_day", "tithi_last_day", "tithi_first_serial", "tithi_days",
    "tithi_names",     "holidays",       "event_days",         "events",
    "event_names",     "names",          "name_bytes",
};

inline constexpr std::uint32_t kAllSummaryKeys =
    static_cast<std::uint32_t>((1U << kSummaryKeys.size()) - 1U);

inline int key_index(std::string_view key) {
    for (std::size_t i = 0; i < kSummaryKeys.size(); ++i) {
        if (kSummaryKeys[i] == key) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

inline void skip_whitespace(std::string_view text, std::size_t& at) {
    while (at < text.size()) {
        const char c = text[at];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return;
        }
        ++at;
    }
}

inline bool take(std::string_view text, std::size_t& at, char expected) {
    skip_whitespace(text, at);
    if (at < text.size() && text[at] == expected) {
        ++at;
        return true;
    }
    return false;
}

inline bool parse_string(std::string_view text, std::size_t& at, std::string& out) {
    if (!take(text, at, '"')) {
        return false;
    }
    out.clear();
    while (at < text.size()) {
        const char c = text[at++];
        if (c == '"') {
            return true;
        }
        if (c == '\\') {
            if (at >= text.size()) {
                return false;
            }
            const char escape = text[at++];
            switch (escape) {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            default:
                return false;
            }
            continue;
        }
        out += c;
    }
    return false;
}

inline bool parse_integer(std::string_view text, std::size_t& at, std::int64_t& out) {
    skip_whitespace(text, at);
    const bool negative = at < text.size() && text[at] == '-';
    if (negative) {
        ++at;
    }
    const std::size_t first = at;
    while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
        ++at;
    }
    if (at == first) {
        return false;
    }
    std::uint64_t magnitude = 0;
    const auto [end, ec] = std::from_chars(text.data() + first, text.data() + at, magnitude);
    if (ec != std::errc{} || end != text.data() + at) {
        return false;
    }
    out = negative ? -static_cast<std::int64_t>(magnitude) : static_cast<std::int64_t>(magnitude);
    return true;
}

inline bool set_text(patro_summary& out, const std::string& key, const std::string& value) {
    if (key == "first_day") {
        out.first_day = value;
    } else if (key == "last_day") {
        out.last_day = value;
    } else if (key == "tithi_first_day") {
        out.tithi_first_day = value;
    } else if (key == "tithi_last_day") {
        out.tithi_last_day = value;
    } else {
        return false;
    }
    return true;
}

inline bool set_number(patro_summary& out, const std::string& key, std::int64_t value) {
    if (value < 0 || value > 0xFFFFFFFFLL) {
        return false;
    }
    const auto number = static_cast<std::uint32_t>(value);
    if (key == "schema") {
        out.schema = static_cast<int>(number);
    } else if (key == "first_year") {
        out.first_year = static_cast<int>(number);
    } else if (key == "last_year") {
        out.last_year = static_cast<int>(number);
    } else if (key == "first_serial") {
        out.first_serial = number;
    } else if (key == "last_serial") {
        out.last_serial = number;
    } else if (key == "days") {
        out.days = number;
    } else if (key == "tithi_first_serial") {
        out.tithi_first_serial = number;
    } else if (key == "tithi_days") {
        out.tithi_days = number;
    } else if (key == "tithi_names") {
        out.tithi_names = number;
    } else if (key == "holidays") {
        out.holidays = number;
    } else if (key == "event_days") {
        out.event_days = number;
    } else if (key == "events") {
        out.events = number;
    } else if (key == "event_names") {
        out.event_names = number;
    } else if (key == "names") {
        out.names = number;
    } else if (key == "name_bytes") {
        out.name_bytes = number;
    } else {
        return false;
    }
    return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one branch per JSON token, test helper
inline bool parse(std::string_view text, patro_summary& out) {
    std::size_t at = 0;
    if (!take(text, at, '{')) {
        return false;
    }
    std::uint32_t seen = 0;
    while (true) {
        skip_whitespace(text, at);
        if (at >= text.size()) {
            return false;
        }
        if (text[at] == '}') {
            ++at;
            break;
        }
        std::string key;
        if (!parse_string(text, at, key) || !take(text, at, ':')) {
            return false;
        }
        const int index = key_index(key);
        if (index < 0) {
            return false;
        }
        const auto bit = static_cast<std::uint32_t>(1U << static_cast<unsigned>(index));
        if ((seen & bit) != 0U) {
            return false;
        }
        seen |= bit;
        skip_whitespace(text, at);
        if (at < text.size() && text[at] == '"') {
            std::string value;
            if (!parse_string(text, at, value) || !set_text(out, key, value)) {
                return false;
            }
        } else {
            std::int64_t value = 0;
            if (!parse_integer(text, at, value) || !set_number(out, key, value)) {
                return false;
            }
        }
        if (take(text, at, ',')) {
            continue;
        }
        if (take(text, at, '}')) {
            break;
        }
        return false;
    }
    skip_whitespace(text, at);
    return at == text.size() && seen == kAllSummaryKeys;
}

} // namespace detail

// Reads and parses the summary; nullopt when the file is missing or malformed.
inline std::optional<patro_summary> read_patro_summary(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    patro_summary summary;
    if (!detail::parse(text, summary)) {
        return std::nullopt;
    }
    return summary;
}

} // namespace nepdate::test

#endif // NEPDATE_TESTS_PATRO_SUMMARY_HPP
