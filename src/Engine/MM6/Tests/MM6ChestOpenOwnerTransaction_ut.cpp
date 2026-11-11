#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestOpenOwnerTransaction.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Tables/ChestTable.h"

namespace {

struct TableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();
    TableScope() {
        pItemTable = table.get();
        initializeChests();
        table->itemSizes[static_cast<ItemId>(200)] = Sizei(1, 1);
        table->items[static_cast<ItemId>(200)].type = ITEM_TYPE_RING;
    }
    ~TableScope() { pItemTable = previous; }
};

void put32(std::string *bytes, size_t offset, std::uint32_t value) {
    for (int byte = 0; byte < 4; ++byte)
        (*bytes)[offset + byte] = static_cast<char>(value >> (8 * byte));
}

std::vector<std::uint8_t> sidecar() {
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    std::string bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 + 56, '\0');
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, 200);
    bytes[chestOffset + 4 + 26] = static_cast<char>(0x5a);
    bytes[0] = 3;
    return {bytes.begin(), bytes.end()};
}

std::vector<std::uint8_t> indoorSidecar() {
    constexpr size_t chestOffset = 8 + 875 + 3 * 4;
    std::string bytes(chestOffset + 20 * mm6::CHEST_WIRE_SIZE + 200 * 80 + 200 + 56, '\0');
    put32(&bytes, chestOffset - 4, 20);
    put32(&bytes, chestOffset + 4, 200);
    bytes[0] = 3;
    return {bytes.begin(), bytes.end()};
}

std::array<std::uint8_t, 292> chestTypes() {
    std::array<std::uint8_t, 292> types{};
    types[0] = 8;
    for (int index = 0; index < 8; ++index)
        types[4 + index * 36 + 34] = index + 1;
    return types;
}

std::vector<Chest> initialChests(const std::vector<std::uint8_t> &bytes) {
    const auto wire = mm6::decodeOutdoorDelta(bytes);
    mm6::ChestRecordSet records{};
    EXPECT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    std::vector<Chest> result(20);
    for (int index = 0; index < 20; ++index)
        reconstruct(projectMM6ChestRecord(records[index]), &result[index], tags::context<int>(index));
    result[0].position = Vec3f(10, 20, 30);
    return result;
}

mm6::ChestRecord firstRecord(const std::vector<std::uint8_t> &bytes) {
    const auto wire = mm6::decodeOutdoorDelta(bytes);
    mm6::ChestRecordSet records{};
    EXPECT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    return records[0];
}

}  // namespace

UNIT_TEST(MM6ChestOpenOwnerTransaction, FirstOpenCommitsRawSidecarRuntimeAndLcgThenReopensWithoutDraw) {
    TableScope table;
    auto bytes = sidecar();
    auto chests = initialChests(bytes);
    const auto types = chestTypes();
    mm6::ChestTitleState title;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &title, 8, {}));
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &bytes, &title};
    const auto icon = [](int) { return mm6::ChestIconSize{32, 32}; };
    const auto original = bytes;
    const auto staged = mm6::stageChestOpenOwner(binding, 0, types, icon);
    ASSERT_TRUE(staged);
    EXPECT_TRUE(staged->placed);
    EXPECT_EQ(bytes, original);
    EXPECT_EQ(title.lcgState, 8u);
    ASSERT_TRUE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    EXPECT_TRUE(chests[0].Initialized());
    EXPECT_EQ(chests[0].position, Vec3f(10, 20, 30));
    EXPECT_EQ(title.lcgState, staged->nextLcgState);
    EXPECT_EQ(bytes, staged->deltaBytes);
    EXPECT_TRUE(mm6::chestMatchesOpenRecord(chests[0], firstRecord(bytes)));
    const auto openedBytes = bytes;
    const auto openedLcg = title.lcgState;
    ASSERT_TRUE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    EXPECT_EQ(bytes, openedBytes);
    EXPECT_EQ(title.lcgState, openedLcg);
}

UNIT_TEST(MM6ChestOpenOwnerTransaction, TrapForeignTitleAndStaleRuntimeRollBackAllOwners) {
    TableScope table;
    auto bytes = sidecar();
    auto chests = initialChests(bytes);
    const auto types = chestTypes();
    mm6::ChestTitleState title;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &title, 8, {}));
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &bytes, &title};
    const auto icon = [](int) { return mm6::ChestIconSize{32, 32}; };
    const auto original = bytes;
    binding.title = GameTitle::MM7;
    EXPECT_FALSE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    binding.title = GameTitle::MM6;
    chests[0].flags |= CHEST_TRAPPED;
    EXPECT_FALSE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    chests[0].flags &= ~CHEST_TRAPPED;
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    bytes[chestOffset + 2] = 1;
    chests[0].flags |= CHEST_TRAPPED;
    EXPECT_FALSE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    EXPECT_EQ(bytes[chestOffset + 2], 1);
    EXPECT_EQ(title.lcgState, 8u);
    EXPECT_EQ(bytes.size(), original.size());
}

UNIT_TEST(MM6ChestOpenOwnerTransaction, TrapGoesOffOrIsDisarmedAfterPlacement) {
    // A trapped chest places its items first. Then its trap either goes off or is disarmed, and either way it's gone
    // from the chest and from the sidecar.
    TableScope table;
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    const auto types = chestTypes();
    const auto icon = [](int) { return mm6::ChestIconSize{32, 32}; };
    for (int score : {0, 100}) {
        auto bytes = sidecar();
        bytes[chestOffset + 2] = 1;
        auto chests = initialChests(bytes);
        mm6::ChestTitleState title;
        ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &title, 8, {}));
        mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &bytes, &title};
        const auto staged = mm6::stageChestOpenOwner(binding, 0, types, icon, mm6::ChestInteractionAction::OpenOnly, {},
                                                     mm6::ChestTrapCheck{true, 2, score, true});
        ASSERT_TRUE(staged);
        EXPECT_TRUE(staged->placed);
        ASSERT_TRUE(staged->trap);
        EXPECT_EQ(staged->trap->disarmed, score > 0);
        EXPECT_EQ(staged->trapFired, score == 0);
        EXPECT_EQ(staged->nextLcgState, staged->trap->nextLcgState);
        EXPECT_EQ(std::to_underlying(staged->chests[0].flags), std::to_underlying(CHEST_ITEMS_PLACED));
        EXPECT_TRUE(mm6::chestMatchesOpenRecord(staged->chests[0], firstRecord(staged->deltaBytes)));
        EXPECT_EQ(title.lcgState, 8u);
    }
}

UNIT_TEST(MM6ChestOpenOwnerTransaction, TrapStaysArmedWithoutCharacterOrClickedObject) {
    // With no active character the chest only places its items. A trap that would go off while the chest wasn't
    // clicked through a decoration or a face stays armed, and the chest opens.
    TableScope table;
    constexpr size_t chestOffset = 8 + 2 * 88 * 11 + 3 * 4;
    const auto types = chestTypes();
    const auto icon = [](int) { return mm6::ChestIconSize{32, 32}; };
    for (bool activeCharacter : {false, true}) {
        auto bytes = sidecar();
        bytes[chestOffset + 2] = 1;
        auto chests = initialChests(bytes);
        mm6::ChestTitleState title;
        ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &title, 8, {}));
        mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_OUTDOOR, &chests, &bytes, &title};
        const auto staged = mm6::stageChestOpenOwner(binding, 0, types, icon, mm6::ChestInteractionAction::OpenOnly, {},
                                                     mm6::ChestTrapCheck{activeCharacter, 2, 0, false});
        ASSERT_TRUE(staged);
        EXPECT_TRUE(staged->placed);
        EXPECT_EQ(staged->trap.has_value(), activeCharacter);
        EXPECT_FALSE(staged->trapFired);
        EXPECT_EQ(std::to_underlying(staged->chests[0].flags), std::to_underlying(CHEST_TRAPPED | CHEST_ITEMS_PLACED));
        EXPECT_TRUE(mm6::chestMatchesOpenRecord(staged->chests[0], firstRecord(staged->deltaBytes)));
    }
}

UNIT_TEST(MM6ChestOpenOwnerTransaction, IndoorSidecarCommitsOneRecordAndPreservesOtherBytes) {
    TableScope table;
    auto bytes = indoorSidecar();
    const auto original = bytes;
    MM6IndoorSnapshot base;
    base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
    const Blob source = Blob::copy(bytes.data(), bytes.size());
    auto wire = deserializeMM6IndoorDelta(source, base);
    const auto &section = wire.section(MM6IndoorSectionKind::Chests);
    mm6::ChestRecordSet records{};
    ASSERT_TRUE(mm6::copyChestRecords(
        {static_cast<const std::uint8_t *>(section.bytes.data()), section.bytes.size()}, &records));
    std::vector<Chest> chests(20);
    for (int index = 0; index < 20; ++index)
        reconstruct(projectMM6ChestRecord(records[index]), &chests[index], tags::context<int>(index));
    const auto types = chestTypes();
    mm6::ChestTitleState title;
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &title, 8, {}));
    mm6::ChestLiveBinding binding{GameTitle::MM6, LEVEL_INDOOR, &chests, &bytes, &title, &base};
    const auto icon = [](int) { return mm6::ChestIconSize{32, 32}; };
    ASSERT_TRUE(mm6::applyChestOpenOwner(binding, 0, types, icon));
    EXPECT_TRUE(chests[0].Initialized());
    EXPECT_NE(bytes, original);
    EXPECT_EQ(bytes[0], original[0]);
    constexpr size_t persistent = 8 + 875 + 3 * 4 + 20 * mm6::CHEST_WIRE_SIZE + 200 * 80;
    EXPECT_EQ(bytes[persistent], original[persistent]);
}
