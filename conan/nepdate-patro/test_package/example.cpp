#include <nepdate/patro.hpp>

#include <cstdio>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const nepdate::patro::day_info info = nepdate::patro::info(d);
    if (const auto t = info.tithi()) {
        if (t->name(nepdate::lang::en) != "Dashami") {
            std::printf("unexpected tithi\n");
            return 1;
        }
    } else {
        std::printf("no tithi\n");
        return 1;
    }
    if (info.events().empty()) {
        std::printf("no events\n");
        return 1;
    }
    std::printf("nepdate-patro OK\n");
    return 0;
}
