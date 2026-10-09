// C1-02: value() on an error can't be a constant expression (ADR-0003 §2). The test
// compile_fail_value_on_error builds this with NEPDATE_EXPECT_COMPILE_ERROR=1 and must fail;
// compile_fail_control builds it with 0 and must pass, so the failure can't come from a typo.

#include <nepdate/date.hpp>

int main() {
#if NEPDATE_EXPECT_COMPILE_ERROR
    constexpr auto d = nepdate::make_date(2081, 4, 33).value();
#else
    constexpr auto d = nepdate::make_date(2081, 4, 32).value();
#endif
    return d.day() == 32 ? 0 : 1;
}
