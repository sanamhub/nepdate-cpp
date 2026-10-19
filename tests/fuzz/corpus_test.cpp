// The seed corpus under tests/fuzz/corpus/ must be exactly the parse.tsv inputs, and every
// file passes the fuzz property (ADR-0004 section 3). Runs as a normal test on every platform and
// in every preset, no libFuzzer needed.

#include "../vectors.hpp"
#include "fuzz_property.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <set>
#include <string>

TEST_CASE("the fuzz corpus is exactly the parse.tsv inputs and passes the property", "[fuzz]") {
    const nepdate_test::table t = nepdate_test::read_vectors("parse.tsv", '\t');
    REQUIRE(t.rows.size() == 69);
    const std::size_t input = t.column("input");
    REQUIRE(input < t.header.size());
    std::set<std::string> inputs;
    for (const auto& row : t.rows) {
        inputs.insert(row[input]);
    }
    REQUIRE(inputs.size() == 69);

    std::set<std::string> corpus;
    std::size_t files = 0;
    const std::filesystem::path dir = NEPDATE_FUZZ_CORPUS_DIR;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::ifstream in(entry.path(), std::ios::binary);
        REQUIRE(in.is_open());
        corpus.insert(
            std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
        ++files;
    }
    REQUIRE(files == 69);
    CHECK(corpus == inputs);

    for (const std::string& bytes : corpus) {
        INFO("input: [" << bytes << "]");
        CHECK(nepdate_test::fuzz::parse_property(bytes));
        CHECK(nepdate_test::fuzz::parse_lenient_property(bytes));
    }
}
