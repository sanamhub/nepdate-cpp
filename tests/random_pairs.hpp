#ifndef NEPDATE_TESTS_RANDOM_PAIRS_HPP
#define NEPDATE_TESTS_RANDOM_PAIRS_HPP

// A Catch2 generator of integer pairs from std::mt19937 with a fixed seed. Catch2's own random()
// takes its seed from --rng-seed, which is random by default in v3, and two GENERATE calls would
// give the cross product instead of pairs.

#include <catch2/generators/catch_generators.hpp>

#include <cstdint>
#include <random>
#include <utility>

namespace nepdate_test {

class random_pair_generator final : public Catch::Generators::IGenerator<std::pair<int, int>> {
  public:
    random_pair_generator(int count, int first_lo, int first_hi, int second_lo, int second_hi)
        : left_(count), first_(first_lo, first_hi), second_(second_lo, second_hi) {
        random_pair_generator::next();
    }

    [[nodiscard]] const std::pair<int, int>& get() const override { return current_; }

    bool next() override {
        if (left_ == 0) {
            return false;
        }
        --left_;
        current_.first = first_(engine_);
        current_.second = second_(engine_);
        return true;
    }

  private:
    static constexpr std::uint32_t kSeed = 20811015U;

    int left_;
    std::mt19937 engine_{kSeed};
    std::uniform_int_distribution<int> first_;
    std::uniform_int_distribution<int> second_;
    std::pair<int, int> current_{};
};

// count pairs, the first in [first_lo, first_hi] and the second in [second_lo, second_hi].
inline Catch::Generators::GeneratorWrapper<std::pair<int, int>>
random_pairs(int count, int first_lo, int first_hi, int second_lo, int second_hi) {
    return Catch::Generators::GeneratorWrapper<std::pair<int, int>>(
        Catch::Detail::make_unique<random_pair_generator>(count, first_lo, first_hi, second_lo,
                                                          second_hi));
}

} // namespace nepdate_test

#endif // NEPDATE_TESTS_RANDOM_PAIRS_HPP
