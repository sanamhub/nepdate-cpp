#include <nepdate/nepdate.hpp>

#include <iostream>

int main() {
    const nepdate::result<nepdate::date> strict = nepdate::parse("2081/04/15");
    const nepdate::result<nepdate::date> lenient = nepdate::parse_lenient("2081 Shrawan 15");
    std::cout << nepdate::to_string(strict.value()) << '\n';
    std::cout << nepdate::to_string(lenient.value()) << '\n';
}
