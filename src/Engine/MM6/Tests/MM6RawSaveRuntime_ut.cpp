#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawSaveRuntime.h"

namespace {

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> outdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

Blob syntheticBase(std::string_view name) {
    const auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
    return Blob::copy(bytes.data(), bytes.size());
}

bool validItem(std::int32_t id) { return id >= 1 && id <= 580; }

mm6::RawSaveFields exampleRaw() {
    mm6::RawSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.inventory.ready = true;
    fields.inventory.characters[0].items[137].setWord(0, 10);
    fields.inventory.characters[0].items[137][27] = 0xa5;
    fields.inventory.characters[0].gridWords[0] = 138;
    fields.inventory.characters[1].items[31].setWord(0, 900); // Unreferenced opaque tail.
    fields.heldItem.present = true;
    fields.heldItem.record.setWord(0, 11);
    fields.heldItem.record[27] = 0x7a;
    fields.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    fields.shops.stock.records[123].setWord(0, 12);
    fields.shops.stock.records[123][27] = 0x5b;
    fields.shops.clocks.deadlineWords[mm6::shopClockSlot(3)] = 72000;
    fields.metadata.header.displayName = "Raw stage";
    fields.metadata.header.mapFileName = "synthetic0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.metadata.ui.activeCharacter = 2;
    fields.recovery.playingTicks = 42;
    fields.npcs.activeCount = 399;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"synthetic" + std::to_string(i) + ".dlv", indoorDelta()});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"outside" + std::to_string(i) + ".ddm", outdoorDelta()});
    fields.chest = {17, {}, "synthetic0.dlv", indoorDelta()};
    return fields;
}

std::vector<std::string> namesOf(const mm6::RawSaveFields &fields) {
    std::vector<std::string> result;
    for (const mm6::SavedMapDelta &delta : fields.maps.deltas)
        result.push_back(delta.name);
    return result;
}

} // namespace

UNIT_TEST(MM6RawSaveRuntime, StagesOwnedRawFamiliesAfterAllMapBases) {
    const auto fields = exampleRaw();
    const auto bytes = mm6::encodeRawSave(fields, GameTitle::MM6);
    const auto names = namesOf(fields);
    auto staged = mm6::stageRawTitleOwners(bytes, names, syntheticBase, validItem, GameTitle::MM6);
    EXPECT_TRUE(staged.inventory.ready);
    EXPECT_TRUE(staged.shops.ready());
    EXPECT_TRUE(mm6::validRawHeldCursorOwner(staged.cursor, GameTitle::MM6));
    EXPECT_TRUE(std::holds_alternative<mm6::StagedIndoorMap>(staged.activeMap));
    EXPECT_EQ(staged.inventory.characters[0].items[137][27], 0xa5);
    EXPECT_EQ(staged.inventory.characters[1].items[31].itemNumberRaw(), 900);
    EXPECT_EQ(staged.cursor.record[27], 0x7a);
    EXPECT_EQ(staged.shops.stock.records[123][27], 0x5b);
    EXPECT_EQ(mm6::encodeRawSave(mm6::captureRawTitleOwners(
                                     staged, names, syntheticBase, validItem, GameTitle::MM6),
                                 GameTitle::MM6), bytes);

    staged.inventory.characters[1].items[31][27] = 0x33;
    staged.shops.stock.records[123][27] = 0x44;
    auto cleared = mm6::stageRawHeldCursorClear(staged.cursor, GameTitle::MM6);
    ASSERT_TRUE(cleared);
    staged.cursor = *cleared;
    const auto changed = mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, GameTitle::MM6);
    EXPECT_EQ(changed.inventory.characters[1].items[31][27], 0x33);
    EXPECT_EQ(changed.shops.stock.records[123][27], 0x44);
    EXPECT_FALSE(changed.heldItem.present);
    EXPECT_EQ(changed.heldItem.record[27], 0x7a);
    EXPECT_NE(mm6::encodeRawSave(changed, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, GameTitle::MM7), std::exception);
    staged.shops.stock.records[123].setWord(0, 581);
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, GameTitle::MM6), std::exception);
    staged.shops.stock.records[123].setWord(0, 12);
    staged.saved.maps.deltas.back().bytes.pop_back();
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6RawSaveRuntime, RejectsMapSetAndLateBaseWithoutPublishingOwners) {
    auto fields = exampleRaw();
    const auto names = namesOf(fields);
    const auto bytes = mm6::encodeRawSave(fields, GameTitle::MM6);
    int baseReads = 0;
    fields.maps.deltas[1].name = "absent.dlv";
    EXPECT_THROW((void) mm6::stageRawTitleOwners(
        mm6::encodeRawSave(fields, GameTitle::MM6), names,
        [&baseReads](std::string_view name) { ++baseReads; return syntheticBase(name); },
        validItem, GameTitle::MM6), std::exception);
    EXPECT_EQ(baseReads, 0);

    fields = exampleRaw();
    fields.maps.deltas.back().bytes.pop_back();
    EXPECT_THROW((void) mm6::stageRawTitleOwners(
        mm6::encodeRawSave(fields, GameTitle::MM6), names, syntheticBase,
        validItem, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageRawTitleOwners(bytes, names, syntheticBase,
                                                validItem, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::stageRawTitleOwners(bytes, names,
        [](std::string_view) { return Blob(); }, validItem, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageRawTitleOwners(bytes, names, syntheticBase,
        {}, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6RawSaveRuntime, RejectsInvalidActiveItemReferencesAndTemplates) {
    auto fields = exampleRaw();
    const auto names = namesOf(fields);
    auto expectRejected = [&names](const mm6::RawSaveFields &candidate) {
        EXPECT_THROW((void) mm6::stageRawTitleOwners(
            mm6::encodeRawSave(candidate, GameTitle::MM6), names, syntheticBase,
            validItem, GameTitle::MM6), std::exception);
    };

    fields.inventory.characters[0].gridWords[0] = 139;
    expectRejected(fields);
    fields = exampleRaw();
    fields.inventory.characters[0].equipmentWords[0] = 138; // Same record as grid.
    expectRejected(fields);
    fields = exampleRaw();
    fields.inventory.characters[0].gridWords[1] = static_cast<std::uint32_t>(-3);
    expectRejected(fields);
    fields = exampleRaw();
    fields.shops.stock.records[123].setWord(0, 581);
    expectRejected(fields);
    fields = exampleRaw();
    fields.inventory.characters[0].items[137].setWord(0, 581);
    expectRejected(fields);
    fields = exampleRaw();
    const auto bytes = mm6::encodeRawSave(fields, GameTitle::MM6);
    EXPECT_THROW((void) mm6::stageRawTitleOwners(bytes, names, syntheticBase,
        [](std::int32_t id) { return id != 12; }, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::stageRawTitleOwners(bytes, names, syntheticBase,
        [](std::int32_t id) { return id != 11; }, GameTitle::MM6), std::exception);
}
