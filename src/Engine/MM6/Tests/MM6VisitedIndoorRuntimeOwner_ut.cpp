#include <cstdint>
#include <exception>
#include <variant>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6VisitedIndoorRuntimeOwner.h"
#include "Engine/MM6/Tests/MM6VisitedOwnerFixtures.h"

namespace fixtures = mm6::testing;

UNIT_TEST(MM6VisitedIndoorRuntimeOwner, DetachesTheActiveMapOfAStagedSave) {
    // Leaving an indoor map keeps all of its runtime owners, detached from the live world.
    DecorationTable decorations;
    const auto staged = fixtures::fixtureStagedIndoorSave(fixtures::fixtureIndoorSave(), decorations);
    const auto owner = mm6::stageVisitedIndoorRuntimeOwner(staged, "d0.blv", decorations, fixtures::fixtureNoFaces,
                                                           GameTitle::MM6);
    EXPECT_EQ(owner.mapName, "d0.blv");
    EXPECT_EQ(owner.nativeDelta.name, "d0.dlv");
    EXPECT_EQ(owner.nativeDelta.bytes, staged.saved.raw.chest.rawActiveDelta);
    EXPECT_EQ(owner.chest, staged.saved.raw.chest);
    EXPECT_EQ(owner.event, staged.saved.eventRuntime.maps.front());
    EXPECT_FALSE(owner.faceOverrides.has_value());
    EXPECT_EQ(owner.nativeActors, staged.saved.actors);
    EXPECT_EQ(owner.nativeSprites, staged.saved.sprites);
    EXPECT_EQ(owner.nativeDecorations, staged.saved.decorations);
    EXPECT_EQ(owner.decorations, staged.saved.runtimeDecorations);
    EXPECT_FALSE(owner.canWriteCheckpoint());

    const auto &base = std::get<mm6::StagedIndoorMap>(staged.represented.raw.activeMap).base;
    EXPECT_NO_THROW(mm6::requireVisitedIndoorRuntimeOwner(owner, "d0.blv", base, decorations,
                                                          fixtures::fixtureNoFaces, GameTitle::MM6));
}

UNIT_TEST(MM6VisitedIndoorRuntimeOwner, RefusesWrongIdentityAndSourceDisagreement) {
    DecorationTable decorations;
    const auto staged = fixtures::fixtureStagedIndoorSave(fixtures::fixtureIndoorSave(), decorations);
    const auto &base = std::get<mm6::StagedIndoorMap>(staged.represented.raw.activeMap).base;
    const auto owner = mm6::stageVisitedIndoorRuntimeOwner(staged, "d0.blv", decorations, fixtures::fixtureNoFaces,
                                                           GameTitle::MM6);

    EXPECT_THROW((void) mm6::stageVisitedIndoorRuntimeOwner(staged, "d1.blv", decorations, fixtures::fixtureNoFaces,
                                                            GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageVisitedIndoorRuntimeOwner(staged, "d0.blv", decorations, fixtures::fixtureNoFaces,
                                                            GameTitle::MM7), std::exception);
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(owner, "D0.blv", GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(owner, "oute2.odm", GameTitle::MM6), std::exception);

    auto changed = owner;
    changed.sprites.mapName = "d1.blv";
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(changed, "d0.blv", GameTitle::MM6), std::exception);
    changed = owner;
    changed.nativeDelta.bytes[100] ^= 1;
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(changed, "d0.blv", GameTitle::MM6), std::exception);
    changed = owner;
    changed.nativeActors.count = 1;
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(changed, "d0.blv", GameTitle::MM6), std::exception);

    // Structurally sound, but the DLV reset day disagrees with the EVRT entry.
    changed = owner;
    changed.nativeDelta.bytes[4] = 6;
    changed.chest.rawActiveDelta = changed.nativeDelta.bytes;
    EXPECT_NO_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(changed, "d0.blv", GameTitle::MM6));
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeOwner(changed, "d0.blv", base, decorations,
                                                       fixtures::fixtureNoFaces, GameTitle::MM6), std::exception);

    // Structurally sound, but the BLV has no such decoration.
    changed = owner;
    changed.nativeDecorations = {"d0.blv", 1, std::vector<std::uint8_t>(60, 0x33)};
    changed.decorations.records.push_back({1, 0, 0});
    EXPECT_NO_THROW(mm6::requireVisitedIndoorRuntimeWireOwner(changed, "d0.blv", GameTitle::MM6));
    EXPECT_THROW(mm6::requireVisitedIndoorRuntimeOwner(changed, "d0.blv", base, decorations,
                                                       fixtures::fixtureNoFaces, GameTitle::MM6), std::exception);
}
