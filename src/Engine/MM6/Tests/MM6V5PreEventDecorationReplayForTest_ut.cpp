#include <exception>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5PreEventDecorationReplayForTest.h"

namespace {

mm6::DecorationRuntimeSaveFields savedDecorations() {
    mm6::DecorationRuntimeSaveFields result;
    result.mapName = "oute3.odm";
    LevelDecoration decoration;
    decoration.uDecorationDescID = DECORATION_CAMPFIRE_5;
    decoration.uFlags |= LEVEL_DECORATION_VISIBLE_ON_MAP;
    decoration.eventVarId = 7;
    result.decorations.push_back(decoration);
    return result;
}

} // namespace

UNIT_TEST(MM6V5PreEventDecorationReplayForTest, RequiresEventsAndActorsThenRestoresFlags) {
    auto fields = savedDecorations();
    const LevelDecoration saved = fields.decorations.front();
    std::vector<LevelDecoration> rebuilt(1);
    rebuilt[0].uDecorationDescID = DECORATION_FOUNTAIN_3;
    mm6::testing::ScopedV5PreEventDecorationReplayForTest scope(
        GameTitle::MM6, std::move(fields));
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        false, true, "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        true, false, "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_TRUE(mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        true, true, "oute3.odm", GameTitle::MM6));
    EXPECT_TRUE(scope.published());
    EXPECT_EQ(rebuilt[0].uDecorationDescID, saved.uDecorationDescID);
    EXPECT_EQ(rebuilt[0].uFlags, saved.uFlags);
    EXPECT_EQ(rebuilt[0].eventVarId, saved.eventVarId);
}

UNIT_TEST(MM6V5PreEventDecorationReplayForTest, RefusesChangedSourcePosition) {
    auto fields = savedDecorations();
    std::vector<LevelDecoration> rebuilt(1);
    rebuilt[0].vPosition.x = 1;
    mm6::testing::ScopedV5PreEventDecorationReplayForTest scope(
        GameTitle::MM6, std::move(fields));
    EXPECT_THROW((void) mm6::testing::runArmedV5PreEventDecorationReplayForTest(
        mm6::testing::PreEventCheckpointResult::Continue, rebuilt, &rebuilt,
        true, true, "oute3.odm", GameTitle::MM6), std::exception);
    EXPECT_FALSE(scope.published());
}
