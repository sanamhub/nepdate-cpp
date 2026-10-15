// tools/codegen: writes the generated headers from shared/data and from the name lists in
// names.hpp (ADR-0001 §5).
//
//   nepdate_codegen --shared <shared dir> --out <repository root> [--check]
//
// --check generates in memory, prints each file that differs from the disk and exits 1 without
// writing. Exit 2 means bad arguments, unreadable input or invalid data.

#include "calendar.hpp"
#include "names.hpp"
#include "patro.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct options {
    fs::path shared;
    fs::path out;
    bool check = false;
};

struct output_file {
    fs::path relative;
    std::string content;
};

std::optional<options> parse_args(std::span<char*> args) {
    options opts;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "--check") {
            opts.check = true;
        } else if (arg == "--shared" && i + 1 < args.size()) {
            opts.shared = fs::path(args[++i]);
        } else if (arg == "--out" && i + 1 < args.size()) {
            opts.out = fs::path(args[++i]);
        } else {
            return std::nullopt;
        }
    }
    if (opts.shared.empty() || opts.out.empty()) {
        return std::nullopt;
    }
    return opts;
}

// Binary mode on both sides, so Windows neither adds nor hides a CR.
std::optional<std::string> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool write_file(const fs::path& path, const std::string& content) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(out);
}

} // namespace

int main(int argc, char** argv) {
    const auto opts = parse_args(std::span<char*>(argv, static_cast<std::size_t>(argc)));
    if (!opts) {
        std::cerr << "usage: nepdate_codegen --shared <dir> --out <dir> [--check]\n";
        return 2;
    }

    const fs::path calendar_path = opts->shared / "data" / "calendar" / "bs-calendar.json";
    const auto calendar_json = read_file(calendar_path);
    if (!calendar_json) {
        std::cerr << "cannot read " << calendar_path.generic_string() << '\n';
        return 2;
    }
    const auto parsed = nepdate::codegen::parse_calendar(*calendar_json);
    if (!parsed.error.empty()) {
        std::cerr << parsed.error << '\n';
        return 2;
    }

    const auto built = nepdate::codegen::build_tables(parsed.value);
    if (!built.error.empty()) {
        std::cerr << built.error << '\n';
        return 2;
    }

    // The patro year files, sorted by year (ADR-0001 section 5).
    const fs::path patro_dir = opts->shared / "data" / "patro";
    std::vector<fs::path> patro_paths;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(patro_dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            patro_paths.push_back(entry.path());
        }
    }
    std::sort(patro_paths.begin(), patro_paths.end());
    if (patro_paths.empty()) {
        std::cerr << "no patro year files under " << patro_dir.generic_string() << '\n';
        return 2;
    }
    std::vector<std::string> patro_jsons;
    for (const fs::path& path : patro_paths) {
        const auto text = read_file(path);
        if (!text) {
            std::cerr << "cannot read " << path.generic_string() << '\n';
            return 2;
        }
        patro_jsons.push_back(*text);
    }
    const auto patro_parsed = nepdate::codegen::parse_patro(patro_jsons);
    if (!patro_parsed.error.empty()) {
        std::cerr << patro_parsed.error << '\n';
        return 2;
    }
    const auto patro =
        nepdate::codegen::build_patro_tables(patro_parsed.value, built.value.month_start);
    if (!patro.error.empty()) {
        std::cerr << patro.error << '\n';
        return 2;
    }

    const std::vector<output_file> files = {
        {.relative = fs::path("include/nepdate/detail/calendar_tables.hpp"),
         .content = nepdate::codegen::render_calendar_tables(parsed.value, built.value)},
        {.relative = fs::path("include/nepdate/detail/names_tables.hpp"),
         .content = nepdate::codegen::render_names_tables()},
        {.relative = fs::path("patro/src/patro_tables.hpp"),
         .content = nepdate::codegen::render_patro_tables(patro.value, built.value.month_start)},
        {.relative = fs::path("patro/include/nepdate/patro_config.hpp"),
         .content = nepdate::codegen::render_patro_config(built.value.fingerprint)},
    };

    int status = 0;
    for (const auto& file : files) {
        const fs::path path = opts->out / file.relative;
        // Unchanged files aren't rewritten, so their timestamps don't trigger rebuilds.
        if (read_file(path) == file.content) {
            continue;
        }
        if (opts->check) {
            std::cerr << "out of date: " << path.generic_string() << '\n';
            status = 1;
        } else if (!write_file(path, file.content)) {
            std::cerr << "cannot write " << path.generic_string() << '\n';
            return 2;
        }
    }
    return status;
}
