#include <cstdint>
#include <exception>
#include <string>
#include <variant>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6IndoorWorldPlan.h"
#include "Engine/MM6/MM6RawSaveRuntime.h"

namespace {

void put32(std::string *bytes, std::size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i)
        (*bytes)[offset + i] = static_cast<char>(value >> (8 * i));
}

void put16(std::string *bytes, std::size_t offset, std::uint16_t value) {
    (*bytes)[offset] = static_cast<char>(value);
    (*bytes)[offset + 1] = static_cast<char>(value >> 8);
}

mm6::StagedRawTitleOwners candidate() {
    // Two BLV vertices, one door using the first, no actors, objects or chests.
    std::string base(136 + 10 * 4 + 12, '\0');
    put32(&base, 116, 8);        // Four 16-bit door array words.
    put32(&base, 136, 2);        // Two vertices.
    put32(&base, 164, 200);      // Fixed door slots after four preceding sections.
    std::string delta(8 + 875 + 3 * 4 + 200 * 80 + 8 + 200 + 56, '\0');
    constexpr std::size_t firstDoor = 8 + 875 + 3 * 4;
    put16(&delta, firstDoor + 68, 1); // One vertex reference.
    put16(&delta, firstDoor + 74, 1); // One offset triple.

    mm6::StagedRawTitleOwners raw;
    MM6IndoorSnapshot parsedBase = deserializeMM6IndoorBase(Blob::fromString(std::move(base)));
    MM6IndoorRuntimeDelta parsedDelta = decodeMM6IndoorRuntimeDelta(
        Blob::fromString(delta), parsedBase);
    raw.activeMap = mm6::StagedIndoorMap{"d0.blv", std::move(parsedBase), std::move(parsedDelta)};
    raw.saved.metadata.header.mapFileName = "d0.blv";
    raw.saved.chest.activeDeltaName = "d0.dlv";
    raw.saved.chest.rawActiveDelta.assign(delta.begin(), delta.end());
    return raw;
}

} // namespace

UNIT_TEST(MM6IndoorWorldPlan, ProjectsV2BaseAndDoorReferencesOnCopies) {
    const auto raw = candidate();
    const auto plan = mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6);
    EXPECT_EQ(plan.mapName, "d0.blv");
    EXPECT_EQ(plan.base.vertices.size(), 2);
    ASSERT_EQ(plan.savedDoors.size(), 200);
    ASSERT_EQ(plan.sidecarDoors.size(), 200);
    EXPECT_EQ(plan.savedDoors[0].vertices, std::vector<std::size_t>({0}));
    EXPECT_EQ(plan.sidecarDoors[0].vertices, std::vector<std::size_t>({0}));
    EXPECT_TRUE(plan.savedDoors[0].faces.empty());
    EXPECT_EQ(plan.savedDelta.doorsData.size(), 4);
    EXPECT_EQ(std::get<mm6::StagedIndoorMap>(raw.activeMap).delta.doorsData[0], 0);
}

UNIT_TEST(MM6IndoorWorldPlan, RejectsLateOwnerAndDoorFaultsWithoutChangingPriorPlan) {
    auto raw = candidate();
    const auto prior = mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6);
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM7), std::exception);
    raw.saved.chest.activeDeltaName = "other.dlv";
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6), std::exception);
    raw.saved.chest.activeDeltaName = "d0.dlv";
    raw.activeCounts.actors = 1;
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6), std::exception);
    raw.activeCounts.actors = 0;
    auto &active = std::get<mm6::StagedIndoorMap>(raw.activeMap);
    active.delta.doorsData[0] = 1; // Rejected even if the retained wire still has vertex zero.
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6), std::exception);
    active.delta.doorsData[0] = 0;
    constexpr std::size_t doorData = 8 + 875 + 3 * 4 + 200 * 80;
    raw.saved.chest.rawActiveDelta[doorData] = 1; // Valid vertex, divergent from saved DLV.
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6), std::exception);
    raw.saved.chest.rawActiveDelta[doorData] = 2; // Absent vertex.
    EXPECT_THROW((void) mm6::prepareDetachedRawIndoorWorldPlan(raw, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.savedDoors[0].vertices, std::vector<std::size_t>({0}));
    EXPECT_EQ(prior.sidecarDoors[0].vertices, std::vector<std::size_t>({0}));
}
