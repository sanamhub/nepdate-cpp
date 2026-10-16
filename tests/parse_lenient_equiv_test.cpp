// The split parse_lenient must agree with the reference (reference/parse_lenient_reference.hpp)
// on every parse.tsv row, every corpus file and 200,000 generated inputs.

#include "reference/parse_lenient_reference.hpp"
#include "vectors.hpp"

#include <nepdate/parse.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <random>
#include <string>
#include <string_view>

namespace {

void check_equiv(std::string_view input) {
    INFO("input: [" << input << "]");
    CHECK(nepdate::parse_lenient(input) == nepdate_test::reference::parse_lenient_reference(input));
}

// Deterministic generator: mixes digits (ASCII and Devanagari), month names (ASCII and
// Devanagari), ignored words, era markers, separators and junk bytes, including digits-and-dots
// runs. Seed fixed so the test is reproducible.
std::string generated_input(std::mt19937& rng) {
    constexpr std::array<std::string_view, 12> ascii_numbers = {
        "0", "1", "15", "29", "05", "04", "2080", "2081", "2024", "80", "12345", "9999"};
    constexpr std::array<std::string_view, 7> ne_numbers = {"०",    "५",  "१५", "२०८०",
                                                            "२०८१", "०५", "०४"};
    constexpr std::array<std::string_view, 18> ascii_months = {
        "baishakh", "baisakh", "bai",    "Baishakh", "BAISAKH", "shrawan",
        "Shr",      "shr",     "srawan", "saun",     "ashoj",   "aso",
        "chaitra",  "cha",     "magh",   "poush",    "falgun",  "kartik"};
    constexpr std::array<std::string_view, 12> ne_months = {
        "बैशाख", "वैशाख", "बैसाख", "जेठ", "साउन", "श्रावण", "सावन", "असोज", "चैत", "माघ", "पुष", "फागुन"};
    constexpr std::array<std::string_view, 2> ignored = {"गते", "मिति"};
    constexpr std::array<std::string_view, 10> eras = {"bs", "BS",   "b.s.",  "B.S.",  "b.s",
                                                       "vs", "v.s.", "बि.सं.", "वि.सं.", "बि.सं"};
    constexpr std::array<std::string_view, 11> junk = {"junk", "xyz", "Foo", "bar", "²",  "٢",
                                                       "ſ",    "ı",   "?",   "!",   "abc"};
    constexpr std::array<std::string_view, 9> separators = {"/", "-", ".", ",", " ",
                                                            "_", "।", "|", "\\"};

    std::string out;
    const int pieces = 1 + static_cast<int>(rng() % 8);
    for (int i = 0; i < pieces; ++i) {
        if (i != 0) {
            out += separators[rng() % separators.size()];
        }
        const int kind = static_cast<int>(rng() % 6);
        if (kind == 0) {
            out += ascii_numbers[rng() % ascii_numbers.size()];
        } else if (kind == 1) {
            out += ne_numbers[rng() % ne_numbers.size()];
        } else if (kind == 2) {
            out += ascii_months[rng() % ascii_months.size()];
        } else if (kind == 3) {
            out += ne_months[rng() % ne_months.size()];
        } else if (kind == 4) {
            out += ignored[rng() % ignored.size()];
        } else if (kind == 5 && rng() % 2 == 0) {
            out += eras[rng() % eras.size()];
        } else {
            out += junk[rng() % junk.size()];
        }
    }
    // Every so often make a digits-and-dots run, which must re-split on dots.
    if (rng() % 4 == 0) {
        out += ".15.2080";
    }
    return out;
}

} // namespace

TEST_CASE("split parse_lenient agrees with the reference on parse.tsv", "[parse][equiv]") {
    const nepdate_test::table t = nepdate_test::read_vectors("parse.tsv", '\t');
    REQUIRE(t.rows.size() == 67);
    const std::size_t input = t.column("input");
    for (const auto& row : t.rows) {
        check_equiv(row[input]);
    }
}

TEST_CASE("split parse_lenient agrees with the reference on the fuzz corpus", "[parse][equiv]") {
    const std::filesystem::path dir = NEPDATE_FUZZ_CORPUS_DIR;
    std::size_t files = 0;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::ifstream in(entry.path(), std::ios::binary);
        REQUIRE(in.is_open());
        check_equiv(
            std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
        ++files;
    }
    CHECK(files == 67);
}

TEST_CASE("split parse_lenient agrees with the reference on 200,000 generated inputs",
          "[parse][equiv]") {
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp): a fixed seed keeps the test reproducible
    std::mt19937 rng(0xC7C7'0202U);
    for (int i = 0; i < 200'000; ++i) {
        check_equiv(generated_input(rng));
    }
}
