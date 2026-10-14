#include <nepdate/nepdate.hpp>
#include <nepdate/patro.hpp>

#include <cstdio>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    if (static_cast<int>(ad.year()) != 2024 || static_cast<unsigned>(ad.month()) != 7 ||
        static_cast<unsigned>(ad.day()) != 30) {
        std::printf("conversion failed\n");
        return 1;
    }
    if (nepdate::from_gregorian(ad).value() != d) {
        std::printf("round trip failed\n");
        return 1;
    }
    const nepdate::patro::day_info info = nepdate::patro::info(d);
    if (!info.tithi().has_value() || info.events().empty()) {
        std::printf("patro lookup failed\n");
        return 1;
    }
    std::printf("consumer OK\n");
    return 0;
}
