#include <nepdate/nepdate.hpp>

#include <cstddef>
#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape): an example program lets an exception terminate
int main() {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access): make returns a checked optional
    const nepdate::date_range week = *nepdate::date_range::make(*nepdate::make_date(2081, 4, 10),
                                                                *nepdate::make_date(2081, 4, 16));
    std::cout << nepdate::to_string(week) << '\n';

    std::size_t working = 0;
    for (const nepdate::date d : week.working_days(true)) {
        (void)d;
        ++working;
    }
    std::cout << working << '\n';

    const auto overlap =
        week.intersect(*nepdate::date_range::of_month(2081, nepdate::month::shrawan));
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access): the ranges overlap
    std::cout << nepdate::to_string(*overlap) << '\n';

    const auto rest = week.subtract(nepdate::date_range::single(*nepdate::make_date(2081, 4, 12)));
    std::cout << rest.size() << '\n';
}
