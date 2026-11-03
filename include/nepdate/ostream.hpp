#ifndef NEPDATE_OSTREAM_HPP
#define NEPDATE_OSTREAM_HPP

// operator<< for date and date_range (D-11). Separate from format.hpp so a translation unit that
// only formats into a buffer doesn't include <ostream>.

#include <nepdate/format.hpp>

#include <array>
#include <cstddef>
#include <ostream>

namespace nepdate {

/// Writes d as to_chars does: "2081/04/15".
inline std::ostream& operator<<(std::ostream& out, date d) {
    std::array<char, max_short_size> buf{};
    const std::to_chars_result result = to_chars(buf.data(), buf.data() + buf.size(), d);
    out.write(buf.data(), static_cast<std::streamsize>(result.ptr - buf.data()));
    return out;
}

/// Writes r as to_string(date_range) does: "2081/04/10 - 2081/04/20".
inline std::ostream& operator<<(std::ostream& out, date_range r) {
    std::array<char, max_long_size> buf{};
    const std::to_chars_result result =
        to_chars_range(buf.data(), buf.data() + buf.size(), r, short_format{}, lang::en);
    out.write(buf.data(), static_cast<std::streamsize>(result.ptr - buf.data()));
    return out;
}

} // namespace nepdate

#endif // NEPDATE_OSTREAM_HPP
