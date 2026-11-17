#include <nepdate/nepdate.hpp>

#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape): an example program lets an exception terminate
int main() {
    const nepdate::date d = *nepdate::make_date(2081, 4, 15);
    std::cout << nepdate::to_string(d) << '\n';
    std::cout << nepdate::format_short(d,
                                       {.order = nepdate::date_order::dmy,
                                        .sep = nepdate::separator::dash,
                                        .pad = true},
                                       nepdate::lang::ne)
              << '\n';
    std::cout << nepdate::format_long(d, {.weekday = true}, nepdate::lang::en) << '\n';
    std::cout << nepdate::format_pattern(d, "yyyy-MM-dd", nepdate::lang::en) << '\n';
}
