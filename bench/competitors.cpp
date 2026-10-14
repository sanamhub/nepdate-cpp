// The competitors of docs/competitors.md, wrapped behind one function-pointer interface so the
// benchmark and the "agrees" check call each the same way. Their sources are fetched by
// FetchContent in bench/CMakeLists.txt and compiled only here; nothing is vendored.

#include "competitors.hpp"

#include "bikram.h" // khumnath/nepdate-cli src/bikram.h (GPL-3.0)
extern "C" {
#include "date.h" // aatishnn/nepdate date.h (GPL), compiled as C
}
#include "nepalical/Ad.h" // nepali-bhasa/nepali-calendar (GPL-3.0)
#include "nepalical/Bs.h" // nepali-bhasa/nepali-calendar (GPL-3.0)

namespace {

// ---- nepdate-cli: header-only Bikram class (docs/competitors.md) ----

bool cli_to_ad(int y, int m, int d, int& gy, int& gm, int& gd) {
    Bikram b;
    b.toGregorian(y, m, d, gy, gm, gd);
    return true;
}

bool cli_to_bs(int y, int m, int d, int& by, int& bm, int& bd) {
    Bikram b;
    b.fromGregorian(y, m, d);
    by = b.getYear();
    bm = b.getMonth();
    bd = b.getDay();
    return true;
}

// ---- aatishnn/nepdate: C functions over struct nep_date / struct eng_date ----

bool aatishnn_to_ad(int y, int m, int d, int& gy, int& gm, int& gd) {
    nep_date n{};
    n.year = y;
    n.month = m;
    n.date = d;
    eng_date e{};
    if (nep_to_eng(n, &e) == 0) {
        return false;
    }
    gy = e.year;
    gm = e.month;
    gd = e.date;
    return true;
}

bool aatishnn_to_bs(int y, int m, int d, int& by, int& bm, int& bd) {
    // eng_to_nep prints a message to stderr outside 1944..2033; guard here so the benchmark
    // stays quiet and records "no answer" instead.
    if (y < 1944 || y > 2033) {
        return false;
    }
    eng_date e{};
    e.year = y;
    e.month = m;
    e.date = d;
    nep_date n{};
    if (eng_to_nep(e, &n) == 0) {
        return false;
    }
    by = n.year;
    bm = n.month;
    bd = n.date;
    return true;
}

// ---- nepalical: Bs / Ad classes (docs/competitors.md) ----

bool nepalical_to_ad(int y, int m, int d, int& gy, int& gm, int& gd) {
    const Bs bs(static_cast<unsigned short>(y), static_cast<unsigned short>(m),
                static_cast<unsigned short>(d));
    const Ad ad(bs);
    gy = static_cast<int>(ad.year());
    gm = static_cast<int>(ad.month());
    gd = static_cast<int>(ad.day());
    return true;
}

bool nepalical_to_bs(int y, int m, int d, int& by, int& bm, int& bd) {
    const Ad ad(static_cast<unsigned short>(y), static_cast<unsigned short>(m),
                static_cast<unsigned short>(d));
    const Bs bs(ad);
    by = static_cast<int>(bs.year());
    bm = static_cast<int>(bs.month());
    bd = static_cast<int>(bs.day());
    return true;
}

const nepdate_bench::rival kRivals[] = {
    {"nepdate-cli", cli_to_ad, cli_to_bs},
    {"aatishnn/nepdate", aatishnn_to_ad, aatishnn_to_bs},
    {"nepalical", nepalical_to_ad, nepalical_to_bs},
};

} // namespace

const nepdate_bench::rival* nepdate_bench::rivals() noexcept {
    return kRivals;
}

int nepdate_bench::rival_count() noexcept {
    return static_cast<int>(sizeof(kRivals) / sizeof(kRivals[0]));
}
