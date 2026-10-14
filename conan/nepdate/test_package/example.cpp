#include <nepdate/nepdate.hpp>

#include <cstdio>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    if (ad != std::chrono::year_month_day{std::chrono::year{2024}, std::chrono::month{7},
                                          std::chrono::day{30}}) {
        std::printf("unexpected AD date\n");
        return 1;
    }
    if (nepdate::from_gregorian(ad).value() != d) {
        std::printf("round trip failed\n");
        return 1;
    }
    std::printf("nepdate OK\n");
    return 0;
}
