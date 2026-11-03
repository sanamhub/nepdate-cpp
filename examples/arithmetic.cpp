#include <nepdate/nepdate.hpp>

#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape): an example program lets an exception terminate
int main() {
    const nepdate::date d = *nepdate::make_date(2081, 4, 32);
    std::cout << nepdate::to_string(*d.add_months(2, nepdate::overflow::clamp)) << '\n';
    std::cout << nepdate::to_string(*d.add_months(2, nepdate::overflow::spill)) << '\n';
    std::cout << d.days_until(*nepdate::make_date(2081, 5, 1)) << '\n';
    std::cout << (d - *nepdate::make_date(2081, 5, 1)).total_days << '\n';
}
