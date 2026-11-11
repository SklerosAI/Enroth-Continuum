#include <array>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6EventRuntimeSaveFields.h"

namespace {

std::vector<LevelDecoration> decorations() {
    std::vector<LevelDecoration> result(3);
    result[0].uDecorationDescID = static_cast<DecorationId>(146);
    result[0].eventVarId = 93; // Pre-PrepareDecorations state must be ignored.
    result[1].uDecorationDescID = static_cast<DecorationId>(167);
    result[1].uEventID = 15; // Explicit-event decoration has no var slot.
    result[2].uDecorationDescID = static_cast<DecorationId>(182);
    result[2].eventVarId = 92;
    return result;
}

mm6::EventRuntimeSaveFields example(std::vector<LevelDecoration> *decorations) {
    mm6::EventRuntimeMapEntry entry;
    entry.mapName = "oute3.odm";
    entry.respawnCount = 1;
    entry.lastRespawnDay = 5;
    entry.mapVars[4] = 9;
    entry.mapVars[74] = 17;
    entry.decorVars[0] = 16;
    entry.decorVars[1] = 12;
    entry.decorVars[123] = 31;
    entry.decorVars[124] = 7;
    entry.bindings = mm6::captureEventRuntimeDecorationBindings(
        *decorations, GameTitle::MM6);
    return {{entry}};
}

} // namespace

UNIT_TEST(MM6EventRuntimeSaveFields, ExactSectionAndPrePrepareBinding) {
    auto live = decorations();
    const auto source = example(&live);
    ASSERT_EQ(source.maps[0].bindings.size(), 2);
    EXPECT_EQ(source.maps[0].bindings[0].eventVarId, 0);
    EXPECT_EQ(source.maps[0].bindings[1].eventVarId, 1);
    EXPECT_EQ(live[0].eventVarId, 93);
    const auto section = mm6::encodeEventRuntimeSection(source, GameTitle::MM6);
    const auto decoded = mm6::decodeEventRuntimeSection({{section}, 4}, GameTitle::MM6);
    EXPECT_EQ(decoded, source);
    EXPECT_EQ(mm6::encodeEventRuntimeSection(decoded, GameTitle::MM6), section);
    EXPECT_EQ(&mm6::requireEventRuntimeMapEntry(
                  decoded, "oute3.odm", 1, 5, live, GameTitle::MM6),
              &decoded.maps[0]);
}

UNIT_TEST(MM6EventRuntimeSaveFields, RefusesMissingStaleAndForeignOwners) {
    auto live = decorations();
    auto source = example(&live);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute4.odm", 1, 5, live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 2, 5, live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 1, 6, live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 1, 5, live, GameTitle::MM7), std::exception);
    live[2].uDecorationDescID = static_cast<DecorationId>(163);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 1, 5, live, GameTitle::MM6), std::exception);
    source.maps[0].lastRespawnDay = 0;
    EXPECT_THROW((void) mm6::encodeEventRuntimeSection(source, GameTitle::MM6),
                 std::exception);
}

UNIT_TEST(MM6EventRuntimeSaveFields, RejectsMalformedAndNoncanonicalPayload) {
    auto live = decorations();
    const auto source = example(&live);
    auto valid = mm6::encodeEventRuntimeSection(source, GameTitle::MM6);
    auto duplicate = source;
    duplicate.maps.push_back(duplicate.maps.front());
    EXPECT_THROW((void) mm6::encodeEventRuntimeSection(duplicate, GameTitle::MM6),
                 std::exception);
    auto badBinding = source;
    badBinding.maps[0].bindings[1].eventVarId = 0;
    EXPECT_THROW((void) mm6::encodeEventRuntimeSection(badBinding, GameTitle::MM6),
                 std::exception);
    EXPECT_THROW((void) mm6::encodeEventRuntimeSection(source, GameTitle::MM7),
                 std::exception);
    valid.payload.push_back(0);
    EXPECT_THROW((void) mm6::decodeEventRuntimeSection({{valid}, 4}, GameTitle::MM6),
                 std::exception);
    valid.payload.pop_back();
    valid.payload[0] = 1; // The unpublished v1 binding omitted exact position.
    EXPECT_THROW((void) mm6::decodeEventRuntimeSection({{valid}, 4}, GameTitle::MM6),
                 std::exception);
}

UNIT_TEST(MM6EventRuntimeSaveFields, RefusesSameTypeDecorationPositionSwap) {
    std::vector<LevelDecoration> live(2);
    live[0].uDecorationDescID = static_cast<DecorationId>(146);
    live[1].uDecorationDescID = static_cast<DecorationId>(146);
    live[0].vPosition.x = 100.25f;
    live[1].vPosition.x = 200.5f;
    const auto source = example(&live);
    EXPECT_NO_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 1, 5, live, GameTitle::MM6));
    std::swap(live[0].vPosition, live[1].vPosition);
    EXPECT_THROW((void) mm6::requireEventRuntimeMapEntry(
        source, "oute3.odm", 1, 5, live, GameTitle::MM6), std::exception);
}
