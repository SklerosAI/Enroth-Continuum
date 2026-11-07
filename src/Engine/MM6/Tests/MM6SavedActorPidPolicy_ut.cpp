#include <array>
#include <exception>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SavedActorPidPolicy.h"
#include "Engine/Pid.h"

UNIT_TEST(MM6SavedActorPidPolicy, InactiveSlotsRetainRawPidWithoutPublication) {
    std::array<mm6::SaveOverlayField, 50> overlays{};
    overlays[0].packedPid = Pid(OBJECT_Actor, 0).packed();
    overlays[0].animationLength = 0;
    overlays[49].packedPid = Pid(OBJECT_Actor, 12).packed();
    overlays[49].animationLength = -1;
    const auto before = overlays;
    EXPECT_NO_THROW(mm6::requireNoActiveSavedActorOverlays(overlays, GameTitle::MM6));
    EXPECT_EQ(overlays, before);
    EXPECT_THROW(mm6::requireNoActiveSavedActorOverlays(overlays, GameTitle::MM7),
                 std::exception);
}

UNIT_TEST(MM6SavedActorPidPolicy, BoundedActorPidStillFailsWithoutIdentity) {
    std::array<mm6::SaveOverlayField, 50> overlays{};
    overlays[49].packedPid = Pid(OBJECT_Actor, 0).packed();
    overlays[49].animationLength = 1; // A saved actor-count bound of one would pass.
    const auto before = overlays;
    EXPECT_THROW(mm6::requireNoActiveSavedActorOverlays(overlays, GameTitle::MM6),
                 std::exception);
    EXPECT_EQ(overlays, before);

    overlays[49].packedPid = Pid(OBJECT_Sprite, 0).packed();
    EXPECT_THROW(mm6::requireNoActiveSavedActorOverlays(overlays, GameTitle::MM6),
                 std::exception);
}
