#include <algorithm>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Decoration.h"

namespace {

constexpr Vec3f originalRockPosition(-15088, -4288, 208);
constexpr float correctedRockZ = 148;

LevelDecoration *avleeFlyingRock() {
    auto rock = std::ranges::find_if(pLevelDecorations, [](const LevelDecoration &decoration) {
        return decoration.uDecorationDescID == DECORATION_ROCK_110 &&
               decoration.vPosition.x == originalRockPosition.x &&
               decoration.vPosition.y == originalRockPosition.y;
    });
    return rock == pLevelDecorations.end() ? nullptr : &*rock;
}

void expectOriginalIdentityExceptHeight(const LevelDecoration &rock) {
    EXPECT_EQ(rock.uDecorationDescID, DECORATION_ROCK_110);
    EXPECT_EQ(rock.uFlags, LevelDecorationFlags());
    EXPECT_EQ(rock._yawAngle, 0);
    EXPECT_EQ(rock.uCog, 0);
    EXPECT_EQ(rock.uEventID, 0);
    EXPECT_EQ(rock.uTriggerRange, 0);
    EXPECT_EQ(rock.eventVarId, -75);
}

} // namespace

GAME_TEST(MM7AvleeRock, OriginalFlyingRockSitsOnTerrain) {
    game.startNewGame();
    game.teleportTo(MAP_AVLEE, Vec3f(), 0);
    LevelDecoration *rock = avleeFlyingRock();
    ASSERT_NE(rock, nullptr);
    expectOriginalIdentityExceptHeight(*rock);

    bool onWater = false;
    int faceId = -1;
    const float floor = ODM_GetFloorLevel(rock->vPosition, &onWater, &faceId);
    EXPECT_EQ(floor, correctedRockZ);
    EXPECT_EQ(rock->vPosition.z, floor);
    EXPECT_FALSE(onWater);
}

GAME_TEST(MM7AvleeRock, CorrectionRequiresExactOriginalSignature) {
    game.startNewGame();
    game.teleportTo(MAP_AVLEE, Vec3f(), 0);
    LevelDecoration *rock = avleeFlyingRock();
    ASSERT_NE(rock, nullptr);
    ASSERT_EQ(rock->vPosition.z, correctedRockZ);

    rock->vPosition.z = originalRockPosition.z;
    onMapLoad();
    EXPECT_EQ(rock->vPosition.z, correctedRockZ);

    rock->vPosition.z = originalRockPosition.z;
    rock->_yawAngle = 1; // A patched/custom decoration must remain authoritative.
    onMapLoad();
    EXPECT_EQ(rock->vPosition.z, originalRockPosition.z);
}
