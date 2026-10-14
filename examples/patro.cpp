#include <nepdate/patro.hpp>

#include <cstdio>
#include <string>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const nepdate::patro::day_info day = nepdate::patro::info(d);
    if (const auto t = day.tithi()) {
        std::printf("%s\n", std::string(t->name(nepdate::lang::en)).c_str());
    }
    std::printf("holiday: %d\n", day.is_public_holiday() ? 1 : 0);
    for (const nepdate::patro::event e : day.events()) {
        std::printf("%s\n", std::string(e.name(nepdate::lang::en)).c_str());
    }
}
