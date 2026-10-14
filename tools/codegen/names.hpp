#ifndef NEPDATE_CODEGEN_NAMES_HPP
#define NEPDATE_CODEGEN_NAMES_HPP

// The source lists of include/nepdate/detail/names_tables.hpp, in UTF-8. The library headers must
// be ASCII (ADR-0001 section 6), so the generator turns these into octal escapes. The codegen tests
// compare every list here with the tables in shared/spec/FORMATTING.md and PARSING.md.

#include <array>
#include <string>
#include <string_view>

namespace nepdate::codegen {

// FORMATTING section 1, months in calendar order.
inline constexpr std::array<std::string_view, 12> kMonthNamesEn = {
    "Baishakh", "Jestha",  "Ashad", "Shrawan", "Bhadra", "Ashoj",
    "Kartik",   "Mangsir", "Poush", "Magh",    "Falgun", "Chaitra",
};
inline constexpr std::array<std::string_view, 12> kMonthShortEn = {
    "Bai", "Jes", "Asa", "Shr", "Bha", "Aso", "Kar", "Man", "Pou", "Mag", "Fal", "Cha",
};
inline constexpr std::array<std::string_view, 12> kMonthNamesNe = {
    "बैशाख", "जेठ", "असार", "साउन", "भदौ", "असोज", "कार्तिक", "मंसिर", "पुष", "माघ", "फागुन", "चैत",
};

// FORMATTING section 1, weekdays from Sunday.
inline constexpr std::array<std::string_view, 7> kWeekdayNamesEn = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
};
inline constexpr std::array<std::string_view, 7> kWeekdayShortEn = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};
inline constexpr std::array<std::string_view, 7> kWeekdayNamesNe = {
    "आइतवार", "सोमवार", "मङ्गलवार", "बुधवार", "बिहिवार", "शुक्रवार", "शनिवार",
};
inline constexpr std::array<std::string_view, 7> kWeekdayShortNe = {
    "आइत", "सोम", "मङ्गल", "बुध", "बिहि", "शुक्र", "शनि",
};

// PARSING section 3, the "Accepted tokens" cell of each month's row, exactly as the spec writes
// it (duplicates included). The generated table drops the duplicates.
inline constexpr std::array<std::string_view, 12> kMonthSpellingRows = {
    "baishakh, baisakh, baishak, baisakh, vaishakh, vaisakh, bai, बैशाख, वैशाख, बैसाख",
    "jestha, jeth, jyestha, jes, जेठ, जेष्ठ, ज्येष्ठ",
    "ashad, ashadh, asar, asadh, asa, असार, आषाढ, असाढ",
    "shrawan, shravan, srawan, sawan, saun, shr, साउन, श्रावण, सावन",
    "bhadra, bhadau, bhadra, bha, भदौ, भाद्र, भाद्रपद",
    "ashoj, asoj, ashwin, aso, असोज, आश्विन, असौज",
    "kartik, kattik, kar, कार्तिक, कात्तिक",
    "mangsir, mansir, margashirsha, man, मंसिर, मङ्सिर, मार्गशीर्ष",
    "poush, paush, push, pus, pou, पुष, पौष, पुस",
    "magh, mag, माघ",
    "falgun, phalgun, fagun, fal, फागुन, फाल्गुन",
    "chaitra, chait, cha, चैत, चैत्र",
};

// PARSING section 2: era markers (Latin ones compare case-insensitively) and the two Nepali words
// lenient parsing drops.
inline constexpr std::array<std::string_view, 10> kEraMarkers = {
    "bs", "b.s.", "b.s", "vs", "v.s.", "v.s", "बि.सं.", "वि.सं.", "बि.सं", "वि.सं",
};
inline constexpr std::array<std::string_view, 2> kDroppedWords = {"गते", "मिति"};

// The bytes of include/nepdate/detail/names_tables.hpp: ASCII only, LF line endings.
[[nodiscard]] std::string render_names_tables();

} // namespace nepdate::codegen

#endif // NEPDATE_CODEGEN_NAMES_HPP
