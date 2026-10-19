#include <nepdate/nepdate.hpp>

#include <iostream>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    std::cout << nepdate::to_string(d) << '\n';
    std::cout << static_cast<int>(ad.year()) << '-' << static_cast<unsigned>(ad.month()) << '-'
              << static_cast<unsigned>(ad.day()) << '\n';
    std::cout << nepdate::to_string(nepdate::from_gregorian(ad).value()) << '\n';
}
