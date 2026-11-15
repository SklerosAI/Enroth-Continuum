#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Library/Platform/Interface/PlatformEnums.h"

#include "MM7GameplayHarness.h"

GAME_TEST(MM7GameplayHarness, ForwardTurnAndStrafeUseRealInput) {
    game.startNewGame();
    engine->config->debug.NoActors.setValue(true);
    game.teleportTo(MAP_HARMONDALE, Vec3f(-5700, 11100, 1), 0);
    game.tick(2); // Let the outdoor floor and party velocity settle before measuring input.

    mm7::testing::MM7MovementResult forward = mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_UP,
        .heldTicks = 10,
        .minimumHorizontalDistance = 100,
    });
    SCOPED_TRACE(forward.diagnostic());
    EXPECT_TRUE(forward.pressPosted);
    EXPECT_TRUE(forward.releasePosted);
    EXPECT_TRUE(forward.releaseProcessed);
    EXPECT_EQ(forward.heldTicks, 10);
    EXPECT_EQ(forward.startMap, MAP_HARMONDALE);
    EXPECT_EQ(forward.endMap, MAP_HARMONDALE);

    mm7::testing::MM7MovementResult turn = mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_LEFT,
        .heldTicks = 6,
        .minimumYawChange = 64,
    });
    SCOPED_TRACE(turn.diagnostic());
    EXPECT_LT(turn.horizontalDistance(), 1);
    EXPECT_EQ(turn.postReleaseYawChange(), 0); // The release frame must not keep turning.

    mm7::testing::MM7MovementResult strafe = mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_LEFTBRACKET,
        .heldTicks = 8,
        .minimumHorizontalDistance = 50,
    });
    SCOPED_TRACE(strafe.diagnostic());
    EXPECT_EQ(strafe.startMap, MAP_HARMONDALE);
    EXPECT_EQ(strafe.endMap, MAP_HARMONDALE);
}

GAME_TEST(MM7GameplayHarness, ContractFailureStillReleasesHeldKey) {
    game.startNewGame();
    engine->config->debug.NoActors.setValue(true);
    game.teleportTo(MAP_HARMONDALE, Vec3f(-5700, 11100, 1), 0);
    game.tick(2);

    EXPECT_THROW(mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_LEFT,
        .heldTicks = 2,
        .minimumYawChange = 1024, // Deliberately impossible in two frames, after the release frame is processed.
    }), Exception);
    int yawAfterFailure = pParty->_viewYaw;
    game.tick(3);
    EXPECT_EQ(pParty->_viewYaw, yawAfterFailure); // A leaked held key would keep turning during these frames.
}
