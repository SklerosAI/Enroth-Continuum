#include <cstdint>
#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5FirstMapLoadAttempt.h"

UNIT_TEST(MM6V5FirstMapLoadAttempt, NumberedSlotReachesStaging) {
    // A numbered slot name passes the name check and is refused only by staging, which needs the MM6 engine.
    const std::vector<std::uint8_t> bytes;
    EXPECT_THROW((void) mm6::V5FirstMapLoadAttempt::fromNativeSlot(bytes, "save000.ec6", GameTitle::MM6), std::runtime_error);
    EXPECT_THROW((void) mm6::V5FirstMapLoadAttempt::fromNativeSlot(bytes, "save999.ec6", GameTitle::MM6), std::runtime_error);
    EXPECT_THROW((void) mm6::V5FirstMapLoadAttempt::fromNativeSlot(bytes, "candidate-v5-after-action.ec6", GameTitle::MM6),
                 std::runtime_error);
}

UNIT_TEST(MM6V5FirstMapLoadAttempt, OtherSlotNamesAndTitlesAreRefused) {
    // Only numbered `saveNNN.ec6` slots and the disposable candidate may start a first-map load.
    const std::vector<std::uint8_t> bytes;
    for (const char *name : {"", "autosave.ec6", "quicksave0.ec6", "save000.mm7", "save00.ec6", "save0000.ec6",
                             "save00a.ec6", "SAVE000.ec6", "save000.EC6", "save000.ec6.bak", "../save000.ec6",
                             "saves/save000.ec6", "candidate-v4.ec6"})
        EXPECT_THROW((void) mm6::V5FirstMapLoadAttempt::fromNativeSlot(bytes, name, GameTitle::MM6), std::invalid_argument) << name;
    EXPECT_THROW((void) mm6::V5FirstMapLoadAttempt::fromNativeSlot(bytes, "save000.ec6", GameTitle::MM7), std::invalid_argument);
}
