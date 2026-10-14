#include <nepdate/nepdate.hpp>

#include <cstdio>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    std::printf("%s\n", nepdate::to_string(d).c_str());
    std::printf("%d-%02u-%02u\n", static_cast<int>(ad.year()), static_cast<unsigned>(ad.month()),
                static_cast<unsigned>(ad.day()));
    std::printf("%s\n", nepdate::to_string(nepdate::from_gregorian(ad).value()).c_str());
}
