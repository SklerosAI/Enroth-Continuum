#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6EventRuntimeRestoreForTest.h"
#include "Engine/Objects/Decoration.h"

namespace {

std::vector<LevelDecoration> decorations() {
    std::vector<LevelDecoration> result(2);
    result[0].uDecorationDescID = static_cast<DecorationId>(146);
    result[1].uDecorationDescID = static_cast<DecorationId>(167);
    return result;
}

mm6::EventRuntimeSaveFields staged(std::span<LevelDecoration> live) {
    mm6::EventRuntimeMapEntry entry;
    entry.mapName = "oute3.odm";
    entry.respawnCount = 1;
    entry.lastRespawnDay = 1;
    entry.mapVars[4] = 7;
    entry.decorVars[1] = 32;
    entry.bindings = mm6::captureEventRuntimeDecorationBindings(live, GameTitle::MM6);
    return {{entry}};
}

} // namespace

UNIT_TEST(MM6EventRuntimeRestoreForTest, ExactNonrespawnRestoresBothOwnersOnce) {
    auto live = decorations();
    PersistentVariables destination{};
    destination.mapVars[0] = 99;
    destination.decorVars[0] = 98;
    mm6::testing::ScopedEventRuntimeRestoreForTest scope(
        GameTitle::MM6, staged(live));
    EXPECT_TRUE(mm6::testing::runArmedEventRuntimeRestoreForTest(
        GameTitle::MM6, "oute3.odm", 1, 1, false, &destination, live));
    EXPECT_TRUE(scope.restored());
    EXPECT_EQ(destination.mapVars[4], 7);
    EXPECT_EQ(destination.mapVars[0], 0);
    EXPECT_EQ(destination.decorVars[1], 32);
    EXPECT_EQ(destination.decorVars[0], 0);
    EXPECT_THROW(mm6::testing::runArmedEventRuntimeRestoreForTest(
        GameTitle::MM6, "oute3.odm", 1, 1, false, &destination, live),
        mm6::testing::EventRuntimeRestoreRefusalForTest);
}

UNIT_TEST(MM6EventRuntimeRestoreForTest, RefusesMismatchBeforePublication) {
    auto live = decorations();
    PersistentVariables destination{};
    destination.mapVars[0] = 99;
    const auto before = destination;
    {
        mm6::testing::ScopedEventRuntimeRestoreForTest scope(
            GameTitle::MM6, staged(live));
        EXPECT_THROW(mm6::testing::runArmedEventRuntimeRestoreForTest(
            GameTitle::MM6, "oute3.odm", 1, 1, true, &destination, live),
            mm6::testing::EventRuntimeRestoreRefusalForTest);
        EXPECT_FALSE(scope.restored());
        EXPECT_EQ(destination.mapVars, before.mapVars);
        EXPECT_EQ(destination.decorVars, before.decorVars);
    }
    {
        mm6::testing::ScopedEventRuntimeRestoreForTest scope(
            GameTitle::MM6, staged(live));
        EXPECT_THROW(mm6::testing::runArmedEventRuntimeRestoreForTest(
            GameTitle::MM6, "oute3.odm", 2, 1, false, &destination, live),
            mm6::testing::EventRuntimeRestoreRefusalForTest);
        EXPECT_FALSE(scope.restored());
        EXPECT_EQ(destination.mapVars, before.mapVars);
        EXPECT_EQ(destination.decorVars, before.decorVars);
    }
    live[0].uDecorationDescID = static_cast<DecorationId>(167);
    {
        auto expectedDecorations = decorations();
        mm6::testing::ScopedEventRuntimeRestoreForTest scope(
            GameTitle::MM6, staged(expectedDecorations));
        EXPECT_THROW(mm6::testing::runArmedEventRuntimeRestoreForTest(
            GameTitle::MM6, "oute3.odm", 1, 1, false, &destination, live),
            mm6::testing::EventRuntimeRestoreRefusalForTest);
        EXPECT_FALSE(scope.restored());
        EXPECT_EQ(destination.mapVars, before.mapVars);
        EXPECT_EQ(destination.decorVars, before.decorVars);
    }
}

UNIT_TEST(MM6EventRuntimeRestoreForTest, UnarmedAndForeignTitleCannotPublish) {
    auto live = decorations();
    PersistentVariables destination{};
    EXPECT_FALSE(mm6::testing::runArmedEventRuntimeRestoreForTest(
        GameTitle::MM6, "oute3.odm", 1, 1, false, &destination, live));
    EXPECT_THROW(mm6::testing::ScopedEventRuntimeRestoreForTest(
        GameTitle::MM7, staged(live)), std::exception);
    mm6::testing::ScopedEventRuntimeRestoreForTest scope(
        GameTitle::MM6, staged(live));
    EXPECT_THROW(mm6::testing::runArmedEventRuntimeRestoreForTest(
        GameTitle::MM7, "oute3.odm", 1, 1, false, &destination, live),
        mm6::testing::EventRuntimeRestoreRefusalForTest);
    EXPECT_FALSE(scope.restored());
}
