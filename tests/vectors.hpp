#ifndef NEPDATE_TESTS_VECTORS_HPP
#define NEPDATE_TESTS_VECTORS_HPP

// Reads the shared vector files (shared/spec/vectors/). Test code, so it may allocate.

#include <cstddef>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace nepdate_test {

struct table {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    // Index of a header column; header.size() when it is missing.
    [[nodiscard]] std::size_t column(std::string_view name) const {
        std::size_t i = 0;
        while (i < header.size() && header[i] != name) {
            ++i;
        }
        return i;
    }
};

inline std::vector<std::string> split(const std::string& line, char separator) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const std::size_t end = line.find(separator, start);
        fields.push_back(line.substr(start, end - start));
        if (end == std::string::npos) {
            return fields;
        }
        start = end + 1;
    }
}

// Reads <shared>/spec/vectors/<relative>. The first line that isn't a # comment is the header.
// An unreadable file gives an empty table, which the row-count checks then reject.
inline table read_vectors(std::string_view relative, char separator) {
    std::ifstream in(std::string(NEPDATE_SHARED_DIR) + "/spec/vectors/" + std::string(relative),
                     std::ios::binary);
    table t;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        if (t.header.empty()) {
            t.header = split(line, separator);
        } else {
            t.rows.push_back(split(line, separator));
        }
    }
    return t;
}

// "2081-04-15" -> {2081, 4, 15}.
struct ymd {
    int year;
    int month;
    int day;
};

inline ymd parse_ymd(const std::string& text) {
    return ymd{std::stoi(text.substr(0, 4)), std::stoi(text.substr(5, 2)),
               std::stoi(text.substr(8, 2))};
}

// "Sunday" -> 0 .. "Saturday" -> 6, or 7 for anything else.
inline unsigned weekday_index(std::string_view name) {
    constexpr std::string_view names[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                                          "Thursday", "Friday", "Saturday"};
    unsigned i = 0;
    while (i < 7 && names[i] != name) {
        ++i;
    }
    return i;
}

} // namespace nepdate_test

#endif // NEPDATE_TESTS_VECTORS_HPP
