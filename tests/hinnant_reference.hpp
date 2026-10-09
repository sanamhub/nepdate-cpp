#ifndef NEPDATE_TESTS_HINNANT_REFERENCE_HPP
#define NEPDATE_TESTS_HINNANT_REFERENCE_HPP

// ALGORITHM section 5, typed line by line (Howard Hinnant, public domain). The library uses
// Neri-Schneider instead; the exhaustive test checks that both agree on every serial.
// Every value is non-negative in the supported range, so int division matches the spec.

namespace nepdate_test {

struct civil_date {
    int y;
    int m;
    int d;
};

constexpr int days_from_civil(int y, int m, int d) {
    if (m <= 2) {
        y = y - 1;
    }
    const int era = y / 400;
    const int yoe = y - era * 400;
    const int mp = (m + 9) % 12;
    const int doy = (153 * mp + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

constexpr civil_date civil_from_days(int z) {
    z = z + 719468;
    const int era = z / 146097;
    const int doe = z - era * 146097;
    const int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = yoe + era * 400;
    const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int mp = (5 * doy + 2) / 153;
    const int d = doy - (153 * mp + 2) / 5 + 1;
    const int m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2) {
        y = y + 1;
    }
    return civil_date{y, m, d};
}

} // namespace nepdate_test

#endif // NEPDATE_TESTS_HINNANT_REFERENCE_HPP
