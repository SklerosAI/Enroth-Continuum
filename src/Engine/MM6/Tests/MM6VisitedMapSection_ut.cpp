#include <cstdint>
#include <exception>
#include <utility>
#include <variant>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V6OutdoorSaveFields.h"
#include "Engine/MM6/MM6VisitedMapSection.h"
#include "Engine/MM6/Tests/MM6VisitedOwnerFixtures.h"

namespace fixtures = mm6::testing;

static mm6::VisitedIndoorRuntimeOwner visitedIndoor(std::string_view mapName) {
    DecorationTable decorations;
    const auto staged = fixtures::fixtureStagedIndoorSave(fixtures::fixtureIndoorSave(), decorations);
    const auto owner = mm6::stageVisitedIndoorRuntimeOwner(staged, "d0.blv", decorations, fixtures::fixtureNoFaces,
                                                           GameTitle::MM6);
    return fixtures::renamedVisitedIndoor(owner, mapName);
}

static mm6::VisitedMapCache mixedCache() {
    mm6::VisitedMapCache cache;
    cache.park(fixtures::fixtureVisitedOutdoor("oute2.odm"), GameTitle::MM6);
    auto indoor = visitedIndoor("d1.blv");
    const std::vector<mm6::IndoorFaceState> base{{1, "stone"}};
    const std::vector<mm6::IndoorFaceState> changed{{2, "stone"}};
    indoor.faceOverrides = mm6::captureIndoorFaceOverrides("d1.blv", base, changed, GameTitle::MM6);
    cache.park(indoor, GameTitle::MM6);
    cache.park(visitedIndoor("d2.blv"), GameTitle::MM6);
    return cache;
}

static std::vector<std::uint8_t> reencoded(const mm6::VisitedMapCache &cache, std::string_view activeMap) {
    return mm6::encodeVisitedMapSection(cache, activeMap, GameTitle::MM6).payload;
}

UNIT_TEST(MM6VisitedMapSection, RoundTripsOutdoorAndIndoorOwners) {
    // Outdoor and indoor maps the party left are stored side by side in one save section.
    const auto cache = mixedCache();
    const auto section = mm6::encodeVisitedMapSection(cache, "o0.odm", GameTitle::MM6);
    EXPECT_TRUE(section.required);
    EXPECT_EQ(section.payload[0], 2);
    EXPECT_EQ(section.payload[1], 3);

    const auto decoded = mm6::decodeVisitedMapSection({{section}, 6}, "o0.odm", GameTitle::MM6);
    ASSERT_EQ(decoded.size(), 3u);
    EXPECT_TRUE(std::holds_alternative<mm6::VisitedIndoorRuntimeOwner>(decoded.entries()[0]));
    EXPECT_TRUE(std::holds_alternative<mm6::VisitedIndoorRuntimeOwner>(decoded.entries()[1]));
    EXPECT_TRUE(std::holds_alternative<mm6::VisitedOutdoorRuntimeOwner>(decoded.entries()[2]));
    const auto &faced = std::get<mm6::VisitedIndoorRuntimeOwner>(*decoded.find("d1.blv"));
    ASSERT_TRUE(faced.faceOverrides.has_value());
    EXPECT_EQ(*faced.faceOverrides, *std::get<mm6::VisitedIndoorRuntimeOwner>(*cache.find("d1.blv")).faceOverrides);
    EXPECT_FALSE(std::get<mm6::VisitedIndoorRuntimeOwner>(*decoded.find("d2.blv")).faceOverrides.has_value());
    EXPECT_EQ(std::get<mm6::VisitedIndoorRuntimeOwner>(*decoded.find("d2.blv")).chest,
              std::get<mm6::VisitedIndoorRuntimeOwner>(*cache.find("d2.blv")).chest);
    EXPECT_EQ(reencoded(decoded, "o0.odm"), section.payload);

    const auto empty = mm6::encodeVisitedMapSection({}, "o0.odm", GameTitle::MM6);
    EXPECT_EQ(empty.payload, (std::vector<std::uint8_t>{2, 0}));
    EXPECT_TRUE(mm6::decodeVisitedMapSection({{empty}, 6}, "o0.odm", GameTitle::MM6).empty());
}

UNIT_TEST(MM6VisitedMapSection, RefusesMalformedCollidingAndOtherVersionPayloads) {
    const auto cache = mixedCache();
    const auto section = mm6::encodeVisitedMapSection(cache, "o0.odm", GameTitle::MM6);
    const auto decode = [](mm6::SaveSection candidate, std::string_view activeMap = "o0.odm") {
        return mm6::decodeVisitedMapSection({{std::move(candidate)}, 6}, activeMap, GameTitle::MM6);
    };

    EXPECT_THROW((void) mm6::encodeVisitedMapSection(cache, "d1.blv", GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::encodeVisitedMapSection(cache, "o0.odm", GameTitle::MM7), std::exception);
    EXPECT_THROW((void) decode(section, "oute2.odm"), std::exception);
    EXPECT_THROW((void) mm6::decodeVisitedMapSection({{section}, 6}, "o0.odm", GameTitle::MM7), std::exception);

    auto changed = section;
    changed.payload.pop_back();
    EXPECT_THROW((void) decode(changed), std::exception);
    changed = section;
    changed.payload.push_back(0);
    EXPECT_THROW((void) decode(changed), std::exception);
    changed = section;
    changed.payload[2] = 7; // Entry kind.
    EXPECT_THROW((void) decode(changed), std::exception);
    changed = section;
    changed.required = false;
    EXPECT_THROW((void) decode(changed), std::exception);

    // Outdoor-only VIST version 1 and the combined version 2 don't decode as each other.
    const auto outdoor = fixtures::fixtureVisitedOutdoor("oute2.odm");
    const auto v1 = mm6::encodeVisitedOutdoorSection(std::span(&outdoor, 1), GameTitle::MM6);
    EXPECT_THROW((void) decode(v1), std::exception);
    EXPECT_THROW((void) mm6::decodeVisitedOutdoorSection({{section}, 6}, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6VisitedMapSection, RefusesUnsortedEntries) {
    mm6::VisitedMapCache first;
    first.park(visitedIndoor("d1.blv"), GameTitle::MM6);
    mm6::VisitedMapCache second;
    second.park(visitedIndoor("d2.blv"), GameTitle::MM6);
    const auto a = reencoded(first, "o0.odm");
    const auto b = reencoded(second, "o0.odm");
    std::vector<std::uint8_t> swapped{2, 2};
    swapped.insert(swapped.end(), b.begin() + 2, b.end());
    swapped.insert(swapped.end(), a.begin() + 2, a.end());
    EXPECT_THROW((void) mm6::decodeVisitedMapSection({{{{'V', 'I', 'S', 'T'}, true, swapped}}, 6}, "o0.odm",
                                                     GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6VisitedMapSection, SessionKeepsActiveIndoorBytesExact) {
    const auto active = mm6::encodeV6IndoorSave(fixtures::fixtureIndoorSave(), GameTitle::MM6);
    const auto bytes = mm6::encodeSessionWithVisitedMaps(active, mixedCache(), GameTitle::MM6);
    const auto container = mm6::decodeSaveContainer(bytes);
    EXPECT_EQ(container.version, 6);
    EXPECT_EQ(container.sections.size(), mm6::decodeSaveContainer(active).sections.size() + 1);

    const auto session = mm6::decodeSessionWithVisitedMaps(bytes, GameTitle::MM6);
    EXPECT_EQ(session.branch, mm6::ActiveMapBranch::IndoorV6);
    EXPECT_EQ(session.activeMap, "d0.blv");
    EXPECT_EQ(session.active, active);
    EXPECT_EQ(session.visited.size(), 3u);
    EXPECT_EQ(mm6::encodeSessionWithVisitedMaps(session.active, session.visited, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeV6IndoorSave(bytes, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6VisitedMapSection, SessionKeepsActiveOutdoorBytesExact) {
    const auto active = mm6::encodeV5Save(fixtures::fixtureOutdoorSave(), GameTitle::MM6);
    const auto bytes = mm6::encodeSessionWithVisitedMaps(active, mixedCache(), GameTitle::MM6);
    const auto session = mm6::decodeSessionWithVisitedMaps(bytes, GameTitle::MM6);
    EXPECT_EQ(session.branch, mm6::ActiveMapBranch::OutdoorV5);
    EXPECT_EQ(session.activeMap, "o0.odm");
    EXPECT_EQ(session.active, active);
    EXPECT_EQ(session.visited.size(), 3u);
    EXPECT_THROW((void) mm6::decodeV5Save(bytes, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeV6OutdoorSave(bytes, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6VisitedMapSection, SessionRefusesCollisionAndForeignContainers) {
    const auto active = mm6::encodeV6IndoorSave(fixtures::fixtureIndoorSave(), GameTitle::MM6);
    mm6::VisitedMapCache colliding;
    colliding.park(visitedIndoor("d0.blv"), GameTitle::MM6);
    EXPECT_THROW((void) mm6::encodeSessionWithVisitedMaps(active, colliding, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::encodeSessionWithVisitedMaps(active, {}, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeSessionWithVisitedMaps(active, GameTitle::MM6), std::exception);

    // A visited owner of the active map, smuggled past the encoder.
    auto container = mm6::decodeSaveContainer(active);
    container.sections.push_back(mm6::encodeVisitedMapSection(colliding, "o0.odm", GameTitle::MM6));
    EXPECT_THROW((void) mm6::decodeSessionWithVisitedMaps(mm6::encodeSaveContainer(container), GameTitle::MM6),
                 std::exception);
}
