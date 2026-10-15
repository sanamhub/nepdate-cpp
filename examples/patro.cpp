#include <nepdate/patro.hpp>

#include <iostream>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const nepdate::patro::day_info day = nepdate::patro::info(d);
    if (const auto t = day.tithi()) {
        std::cout << t->name(nepdate::lang::en) << '\n';
    }
    std::cout << "holiday: " << (day.is_public_holiday() ? 1 : 0) << '\n';
    for (const nepdate::patro::event e : day.events()) {
        std::cout << e.name(nepdate::lang::en) << '\n';
    }
}
