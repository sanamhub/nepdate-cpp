#include <nepdate/nepdate.hpp>

#include <iostream>
#include <optional>
#include <system_error>

// NOLINTNEXTLINE(bugprone-exception-escape): an example program lets an exception terminate
int main() {
    const nepdate::result<nepdate::date> parsed = nepdate::parse("2081/04/33");
    if (!parsed) {
        std::cout << nepdate::to_string(parsed.error()) << '\n';
        const std::error_code code = parsed.error();
        std::cout << code.message() << '\n';
    }
    const std::optional<nepdate::date> good = nepdate::try_parse("2081-04-15");
    std::cout << (good.has_value() ? nepdate::to_string(*good) : "none") << '\n';
}
