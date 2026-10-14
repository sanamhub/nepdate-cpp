// C4-03 AC: a header compiled with a fake NEPDATE_PATRO_CALENDAR_FINGERPRINT fails the
// static_assert in patro.hpp. The test target injects the wrong value as a compile definition,
// so the generated patro_config.hpp keeps its own (guarded) definition out of the way.
#include <nepdate/patro.hpp>

int main() {
    return static_cast<int>(nepdate::patro::coverage().size());
}
