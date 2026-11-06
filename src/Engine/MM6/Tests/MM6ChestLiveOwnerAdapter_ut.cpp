#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/ChestTable.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();
    TableScope() {
        pItemTable = table.get();
        initializeChests();
        for (int id = 1; id <= 580; ++id)
            table->itemSizes[static_cast<ItemId>(id)] = Sizei(1, 1);
        table->items[static_cast<ItemId>(1)].type = ITEM_TYPE_RING;
        auto &item = table->items[static_cast<ItemId>(200)];
        item.type = ITEM_TYPE_RING;
        for (int tier = 1; tier <= 6; ++tier)
            item.uChanceByTreasureLvl[static_cast<ItemTreasureLevel>(tier)] = 100;
        for (auto tier : table->standardEnchantmentRangeByTreasureLevel.indices())
            table->standardEnchantmentRangeByTreasureLevel[tier] = {1, 1};
    }
    ~TableScope() { pItemTable = previous; }
};

void put32(std::string *bytes, size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i)
        (*bytes)[offset + i] = static_cast<char>(value >> (8 * i));
}

std::vector<std::uint8_t> outdoorSidecar(int marker = -1) {
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    std::string bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 + 56, '\0');
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, static_cast<std::uint32_t>(marker));
    bytes[0] = 3;  // Opaque reset and persistent bytes must survive.
    bytes[chestOffset + 20 * mm6::CHEST_WIRE_SIZE] = static_cast<char>(0xA5);
    return {bytes.begin(), bytes.end()};
}

std::vector<std::uint8_t> indoorSidecar(int marker = -1) {
    constexpr size_t chestOffset = 8 + 875 + 3 * 4;
    std::string bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 * 80 + 200 + 56, '\0');
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, static_cast<std::uint32_t>(marker));
    bytes[0] = 3;
    bytes[chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 * 80] = static_cast<char>(0xA5);
    return {bytes.begin(), bytes.end()};
}

MM6IndoorSnapshot indoorBaseShape() {
    MM6IndoorSnapshot base;
    base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
    return base;
}

std::vector<Chest> baselineChests(const mm6::ChestRecordSet &records) {
    std::vector<Chest> chests(20);
    for (int index = 0; index < 20; ++index) {
        const Chest_MM7 source = projectMM6ChestRecord(records[index]);
        reconstruct(source, &chests[index], tags::context<int>(index));
    }
    return chests;
}

Blob readFile(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    return Blob::fromString(std::string(std::istreambuf_iterator<char>(stream), {}));
}

mm6::ChestRecordSet recordsFromOutdoor(const std::vector<std::uint8_t> &sidecar) {
    const auto wire = mm6::decodeOutdoorDelta(sidecar);
    mm6::ChestRecordSet records{};
    EXPECT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    return records;
}

mm6::ChestRecordSet recordsFromIndoor(const std::vector<std::uint8_t> &sidecar,
                                     const MM6IndoorSnapshot &base) {
    const Blob blob = Blob::copy(sidecar.data(), sidecar.size());
    const auto wire = deserializeMM6IndoorDelta(blob, base);
    const Blob &part = wire.section(MM6IndoorSectionKind::Chests).bytes;
    mm6::ChestRecordSet records{};
    EXPECT_TRUE(mm6::copyChestRecords(
        {static_cast<const std::uint8_t *>(part.data()), part.size()}, &records));
    return records;
}

}  // namespace

UNIT_TEST(MM6ChestLiveOwnerAdapter, OutdoorCommitsActualOwnerShapeAndPreservesOtherBytes) {
    TableScope table;
    auto sidecar = outdoorSidecar();
    const auto original = sidecar;
    auto chests = baselineChests(recordsFromOutdoor(sidecar));
    mm6::ChestTitleState state;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &sidecar, &state};
    const auto staged = mm6::stageChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {});
    ASSERT_TRUE(staged);
    EXPECT_EQ(sidecar, original);
    EXPECT_EQ(state.lcgState, 8u);
    ASSERT_TRUE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(sidecar, staged->deltaBytes);
    EXPECT_EQ(state.lcgState, staged->nextLcgState);
    EXPECT_EQ(chests.size(), 20);
    const auto records = recordsFromOutdoor(sidecar);
    EXPECT_TRUE(mm6::liveChestsMatchSidecar(GameTitle::MM6, chests, records));
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    EXPECT_EQ(sidecar[0], original[0]);
    EXPECT_EQ(sidecar[chestOffset + 20 * mm6::CHEST_WIRE_SIZE], original[chestOffset + 20 * mm6::CHEST_WIRE_SIZE]);
}

UNIT_TEST(MM6ChestLiveOwnerAdapter, IndoorCommitsPatchedSidecarAndArtifactState) {
    TableScope table;
    auto sidecar = indoorSidecar(-7);
    const auto original = sidecar;
    const auto base = indoorBaseShape();
    auto chests = baselineChests(recordsFromIndoor(sidecar, base));
    mm6::ChestTitleState state;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_INDOOR, &chests, &sidecar, &state, &base};
    ASSERT_TRUE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    const auto records = recordsFromIndoor(sidecar, base);
    const int id = mm6::readChest32(std::span(records[0]).subspan(4, 28), 0);
    EXPECT_GE(id, 400);
    EXPECT_LE(id, 429);
    EXPECT_EQ(state.generatedArtifactFlags[id - 400], 1);
    EXPECT_TRUE(mm6::liveChestsMatchSidecar(GameTitle::MM6, chests, records));
    EXPECT_EQ(sidecar[0], original[0]);
    constexpr size_t persistent = 8 + 875 + 3 * 4 + 20 * mm6::CHEST_WIRE_SIZE + 200 * 80;
    EXPECT_EQ(sidecar[persistent], original[persistent]);
}

UNIT_TEST(MM6ChestLiveOwnerAdapter, LateFailureAndStaleRuntimeLeaveEveryOwnerUntouched) {
    TableScope table;
    auto sidecar = outdoorSidecar(-7);
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    for (int byte = 0; byte < 4; ++byte)
        sidecar[chestOffset + 19 * mm6::CHEST_WIRE_SIZE + 4 + byte] = static_cast<std::uint8_t>(250 >> (8 * byte));
    auto chests = baselineChests(recordsFromOutdoor(sidecar));
    mm6::ChestTitleState state;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    const auto original = sidecar;
    table.table->itemSizes[static_cast<ItemId>(250)] = Sizei();
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &sidecar, &state};
    EXPECT_FALSE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(sidecar, original);
    EXPECT_EQ(state.lcgState, 8u);
    EXPECT_EQ(state.generatedArtifactFlags, (std::array<std::uint8_t, 30>{}));
    EXPECT_EQ(chests.size(), 20);

    table.table->itemSizes[static_cast<ItemId>(250)] = Sizei(1, 1);
    chests[0].flags |= CHEST_TRAPPED;
    EXPECT_FALSE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(sidecar, original);
    EXPECT_EQ(state.lcgState, 8u);

    chests[0].flags &= ~CHEST_TRAPPED;
    chests[0].position = Vec3f{};  // Only the pre-UpdateChestPositions load window is accepted.
    EXPECT_FALSE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_EQ(sidecar, original);
}

UNIT_TEST(MM6ChestLiveOwnerAdapter, ForeignUnreadyAndMalformedStateReject) {
    TableScope table;
    auto sidecar = outdoorSidecar();
    auto chests = baselineChests(recordsFromOutdoor(sidecar));
    mm6::ChestTitleState state;
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &sidecar, &state};
    EXPECT_FALSE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_FALSE(mm6::restoreChestTitleState(GameTitle::MM7, &state, 8, {}));
    std::array<std::uint8_t, 30> malformed{};
    malformed[0] = 2;
    EXPECT_FALSE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, malformed));
    EXPECT_FALSE(state.ready);
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
    binding.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::applyChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_1, *table.table, {}));
    EXPECT_FALSE(mm6::applyActiveChestLiveOwnerHandoff(GameTitle::MM7, MAP_TREASURE_LEVEL_1,
                                                       *table.table, {}));
    mm6::resetChestTitleState(&state);
    EXPECT_FALSE(state.ready);
}

UNIT_TEST(MM6ChestLiveOwnerAdapter, RetainedDecodedMapsMatchCurrentRuntimeChestShape) {
    const char *indoorDirectory = std::getenv("ENROTH_MM6_INDOOR_DECODED_DIR");
    const char *outdoorDirectory = std::getenv("ENROTH_MM6_OUTDOOR_DECODED_DIR");
    if (!indoorDirectory || !outdoorDirectory)
        GTEST_SKIP() << "Set retained decoded MM6 map directories";
    TableScope table;
    int indoorCount = 0;
    int outdoorCount = 0;
    for (const auto &entry : std::filesystem::directory_iterator(indoorDirectory)) {
        if (entry.path().extension() != ".dlv")
            continue;
        auto basePath = entry.path();
        basePath.replace_extension(".blv");
        const auto fullBase = deserializeMM6IndoorBase(readFile(basePath));
        auto base = indoorBaseShape();
        base.doorDataBytes = fullBase.doorDataBytes;
        const Blob delta = readFile(entry.path());
        std::vector<std::uint8_t> sidecar(static_cast<const std::uint8_t *>(delta.data()),
                                          static_cast<const std::uint8_t *>(delta.data()) + delta.size());
        auto chests = baselineChests(recordsFromIndoor(sidecar, base));
        mm6::ChestTitleState state;
        ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
        mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_INDOOR, &chests, &sidecar, &state, &base};
        ASSERT_TRUE(mm6::stageChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_6, *table.table, {}))
            << "indoor ordinal=" << indoorCount;
        ++indoorCount;
    }
    for (const auto &entry : std::filesystem::directory_iterator(outdoorDirectory)) {
        if (entry.path().extension() != ".ddm")
            continue;
        const Blob delta = readFile(entry.path());
        std::vector<std::uint8_t> sidecar(static_cast<const std::uint8_t *>(delta.data()),
                                          static_cast<const std::uint8_t *>(delta.data()) + delta.size());
        auto chests = baselineChests(recordsFromOutdoor(sidecar));
        mm6::ChestTitleState state;
        ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 8, {}));
        mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &sidecar, &state};
        ASSERT_TRUE(mm6::stageChestLiveOwnerHandoff(binding, MAP_TREASURE_LEVEL_6, *table.table, {}))
            << "outdoor ordinal=" << outdoorCount;
        ++outdoorCount;
    }
    EXPECT_EQ(indoorCount, 52);
    EXPECT_EQ(outdoorCount, 15);
}
