#include <nepdate/nepdate.hpp>

#include <cstdio>

int main() {
    const nepdate::result<nepdate::date> strict = nepdate::parse("2081/04/15");
    const nepdate::result<nepdate::date> lenient = nepdate::parse_lenient("2081 Shrawan 15");
    std::printf("%s\n", nepdate::to_string(strict.value()).c_str());
    std::printf("%s\n", nepdate::to_string(lenient.value()).c_str());
}
