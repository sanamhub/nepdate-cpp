// tools/codegen: writes the generated headers from shared/data and from the name lists in
// names.hpp (ADR-0001 §5).
//
//   nepdate_codegen --shared <shared dir> --out <repository root> [--check]
//
// --check generates in memory, prints each file that differs from the disk and exits 1 without
// writing. Exit 2 means bad arguments, unreadable input or invalid data.

#include "calendar.hpp"
#include "names.hpp"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
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
        std::fputs("usage: nepdate_codegen --shared <dir> --out <dir> [--check]\n", stderr);
        return 2;
    }

    const fs::path calendar_path = opts->shared / "data" / "calendar" / "bs-calendar.json";
    const auto calendar_json = read_file(calendar_path);
    if (!calendar_json) {
        std::fprintf(stderr, "cannot read %s\n", calendar_path.generic_string().c_str());
        return 2;
    }
    const auto parsed = nepdate::codegen::parse_calendar(*calendar_json);
    if (!parsed.error.empty()) {
        std::fprintf(stderr, "%s\n", parsed.error.c_str());
        return 2;
    }

    const auto built = nepdate::codegen::build_tables(parsed.value);
    if (!built.error.empty()) {
        std::fprintf(stderr, "%s\n", built.error.c_str());
        return 2;
    }

    const std::vector<output_file> files = {
        {.relative = fs::path("include/nepdate/detail/calendar_tables.hpp"),
         .content = nepdate::codegen::render_calendar_tables(parsed.value, built.value)},
        {.relative = fs::path("include/nepdate/detail/names_tables.hpp"),
         .content = nepdate::codegen::render_names_tables()},
    };

    int status = 0;
    for (const auto& file : files) {
        const fs::path path = opts->out / file.relative;
        // Unchanged files aren't rewritten, so their timestamps don't trigger rebuilds.
        if (read_file(path) == file.content) {
            continue;
        }
        if (opts->check) {
            std::fprintf(stderr, "out of date: %s\n", path.generic_string().c_str());
            status = 1;
        } else if (!write_file(path, file.content)) {
            std::fprintf(stderr, "cannot write %s\n", path.generic_string().c_str());
            return 2;
        }
    }
    return status;
}
