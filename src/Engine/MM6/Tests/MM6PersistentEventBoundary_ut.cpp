#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PersistentEventBoundary.h"

namespace {

mm6::StagedActiveMap indoorWithPersistent(const std::array<std::uint8_t, 200> &bytes) {
    mm6::StagedIndoorMap map;
    map.mapName = "synthetic.blv";
    map.delta.wire.isDelta = true;
    map.delta.wire.sections.push_back({MM6IndoorSectionKind::PersistentBytes,
                                       Blob::copy(bytes.data(), bytes.size()), 0});
    return map;
}

mm6::StagedActiveMap outdoorWithPersistent(const std::array<std::uint8_t, 200> &bytes) {
    mm6::StagedOutdoorMap map;
    map.mapName = "synthetic.odm";
    map.delta.wire.bytes.resize(8 + bytes.size() + 56);
    map.delta.wire.parts[static_cast<std::size_t>(mm6::OutdoorDeltaPart::PersistentBytes)] =
        {8, bytes.size()};
    std::copy(bytes.begin(), bytes.end(), map.delta.wire.bytes.begin() + 8);
    return map;
}

} // namespace

UNIT_TEST(MM6PersistentEventBoundary, RoundTripsProvedMapBytesForIndoorAndOutdoor) {
    std::array<std::uint8_t, 200> source{};
    source[0] = 7;
    source[70] = 31;
    const auto check = [&source](const mm6::StagedActiveMap &map) {
        const auto staged = mm6::stagePersistentEventBoundary(map, GameTitle::MM6);
        EXPECT_EQ(staged.mapBytes[0], 7);
        EXPECT_EQ(staged.mapBytes[70], 31);
        EXPECT_EQ(staged.legacy.mapVars, (std::array<unsigned char, 75>{}));
        EXPECT_EQ(staged.legacy.decorVars, (std::array<unsigned char, 125>{}));
        EXPECT_EQ(mm6::capturePersistentEventBoundary(staged, GameTitle::MM6), source);
    };
    check(indoorWithPersistent(source));
    check(outdoorWithPersistent(source));
}

UNIT_TEST(MM6PersistentEventBoundary, RejectsUnprovedBytesAndLiveLegacyWrites) {
    std::array<std::uint8_t, 200> source{};
    for (std::size_t index : {71u, 75u, 199u}) {
        source[index] = 1;
        EXPECT_THROW((void) mm6::stagePersistentEventBoundary(indoorWithPersistent(source), GameTitle::MM6),
                     std::exception);
        EXPECT_THROW((void) mm6::stagePersistentEventBoundary(outdoorWithPersistent(source), GameTitle::MM6),
                     std::exception);
        source[index] = 0;
    }
    EXPECT_THROW((void) mm6::stagePersistentEventBoundary(indoorWithPersistent(source), GameTitle::MM7),
                 std::exception);
    auto staged = mm6::stagePersistentEventBoundary(indoorWithPersistent(source), GameTitle::MM6);
    staged.legacy.mapVars[0] = 1;
    EXPECT_THROW((void) mm6::capturePersistentEventBoundary(staged, GameTitle::MM6), std::exception);
    staged.legacy.mapVars[0] = 0;
    staged.legacy.decorVars[124] = 1;
    EXPECT_THROW((void) mm6::capturePersistentEventBoundary(staged, GameTitle::MM6), std::exception);
    staged.legacy.decorVars[124] = 0;
    EXPECT_THROW((void) mm6::capturePersistentEventBoundary(staged, GameTitle::MM7), std::exception);

    mm6::StagedIndoorMap missing;
    EXPECT_THROW((void) mm6::stagePersistentEventBoundary(mm6::StagedActiveMap(std::move(missing)),
                                                         GameTitle::MM6), std::exception);
    auto malformed = outdoorWithPersistent(source);
    std::get<mm6::StagedOutdoorMap>(malformed).delta.wire.parts[
        static_cast<std::size_t>(mm6::OutdoorDeltaPart::PersistentBytes)].size = 199;
    EXPECT_THROW((void) mm6::stagePersistentEventBoundary(malformed, GameTitle::MM6), std::exception);
}
