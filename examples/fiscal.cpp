#include <nepdate/nepdate.hpp>

#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape): an example program lets an exception terminate
int main() {
    const nepdate::fiscal_year fy = nepdate::fiscal_year::of(*nepdate::make_date(2081, 4, 15));
    std::cout << fy.label(nepdate::lang::en) << '\n';
    std::cout << nepdate::to_string(*fy.start()) << '\n';
    std::cout << nepdate::to_string(*fy.end()) << '\n';
    std::cout << nepdate::to_string(*fy.quarter_range(nepdate::quarter::q1)) << '\n';
}
