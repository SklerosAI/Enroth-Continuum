#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6RawSaveRuntime.h"
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6WorldSaveRuntime.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/Pid.h"

#ifndef _WIN32
#include <cstdlib>
#endif

namespace {

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200;
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

std::vector<std::uint8_t> indoorDeltaWithActor() {
    std::vector<std::uint8_t> bytes(8 + 875 + 3 * 4 + 548 + 200 * 80 + 200 + 56, 0);
    bytes[8 + 875] = 1;
    return bytes;
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

mm6::NpcStaticBaseline freshNpcBaseline() {
    const auto table = [](std::string_view text) { return Blob::copy(text.data(), text.size()); };
    return mm6::loadNpcStaticBaseline(
        table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
        table("0\tNews\tunused\tTown news\r\n"),
        table("Header\r\nAdam\tBeth\r\n"),
        table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
        GameTitle::MM6);
}

struct ItemTableScope {
    ItemTable *previous = pItemTable;
    std::unique_ptr<ItemTable> table = std::make_unique<ItemTable>();

    ItemTableScope() {
        pItemTable = table.get();
        const ItemId id = static_cast<ItemId>(10);
        table->items[id].iconName = "raw-party-item";
        table->items[id].type = static_cast<ItemType>(3);
        table->items[id].rarity = RARITY_COMMON;
        table->itemSizes[id] = Sizei(1, 1);
    }

    ~ItemTableScope() { pItemTable = previous; }
};

#ifndef _WIN32
class TestProfile {
 public:
    TestProfile() {
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-raw-slot-XXXXXX").string();
        if (!::mkdtemp(pattern.data()))
            throw std::runtime_error("cannot create MM6 raw slot test profile");
        _root = pattern;
    }

    ~TestProfile() {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    const std::filesystem::path &root() const { return _root; }
    std::filesystem::path primary() const { return _root / "saves" / "save001.ec6"; }
    std::filesystem::path backup() const { return _root / "saves" / "save001.ec6.bak"; }

 private:
    std::filesystem::path _root;
};

void writeBytes(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    if (!file)
        throw std::runtime_error("cannot write MM6 raw slot test bytes");
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
#endif

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
    Party sourceParty;
    mm6::IndoorFaceOverrides observedFaces;
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
                                     staged, names, syntheticBase, validItem,
                                     sourceParty, observedFaces, GameTitle::MM6),
                                 GameTitle::MM6), bytes);

    staged.inventory.characters[1].items[31][27] = 0x33;
    staged.shops.stock.records[123][27] = 0x44;
    auto cleared = mm6::stageRawHeldCursorClear(staged.cursor, GameTitle::MM6);
    ASSERT_TRUE(cleared);
    staged.cursor = *cleared;
    const auto changed = mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM6);
    EXPECT_EQ(changed.inventory.characters[1].items[31][27], 0x33);
    EXPECT_EQ(changed.shops.stock.records[123][27], 0x44);
    EXPECT_FALSE(changed.heldItem.present);
    EXPECT_EQ(changed.heldItem.record[27], 0x7a);
    EXPECT_NE(mm6::encodeRawSave(changed, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM7), std::exception);
    staged.shops.stock.records[123].setWord(0, 581);
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM6), std::exception);
    staged.shops.stock.records[123].setWord(0, 12);
    staged.saved.maps.deltas.back().bytes.pop_back();
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6RawSaveRuntime, RawV2AdmissionRejectsBeaconAndIndoorFaceState) {
    ItemTableScope table;
    const auto fields = exampleRaw();
    const auto bytes = mm6::encodeRawSave(fields, GameTitle::MM6);
    const auto names = namesOf(fields);
    auto staged = mm6::stageRawTitleOwners(bytes, names, syntheticBase, validItem, GameTitle::MM6);
    Party sourceParty;
    mm6::IndoorFaceOverrides observedFaces;

    sourceParty.pCharacters[0].vBeacons[0] = LloydBeacon{};
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM6),
        std::exception);
    EXPECT_THROW((void) mm6::stageRawPartyCoreProgressInventory(
        sourceParty, staged, *table.table, GameTitle::MM6), std::exception);
    sourceParty.pCharacters[0].vBeacons[0].reset();

    observedFaces.maps.push_back(mm6::IndoorFaceMapOverrides{});
    EXPECT_THROW((void) mm6::captureRawTitleOwners(
        staged, names, syntheticBase, validItem, sourceParty, observedFaces, GameTitle::MM6),
        std::exception);
    EXPECT_EQ(mm6::encodeRawSave(staged.saved, GameTitle::MM6), bytes);
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

UNIT_TEST(MM6RawSaveRuntime, JoinsCoreProgressAndRawInventoryOnPartyCopy) {
    ItemTableScope table;
    auto fields = exampleRaw();
    fields.core.party.gold = 123;
    fields.bits.quest[0] = 1;
    fields.effects.conditionTicks[2][7] = 42;
    const auto names = namesOf(fields);
    auto owners = mm6::stageRawTitleOwners(mm6::encodeRawSave(fields, GameTitle::MM6),
        names, syntheticBase, validItem, GameTitle::MM6);
    Party baseline;
    baseline.uNumGold = 999;
    auto joined = mm6::stageRawPartyCoreProgressInventory(
        baseline, owners, *table.table, GameTitle::MM6);
    EXPECT_EQ(joined.party.uNumGold, 123);
    EXPECT_TRUE(joined.party._questBits.test(static_cast<QuestBit>(1)));
    EXPECT_EQ(joined.party.pCharacters[2].conditions.get(static_cast<Condition>(7)).ticks(), 42);
    EXPECT_EQ(joined.party.pCharacters[0].inventory.capacity(), 138);
    ASSERT_TRUE(joined.party.pCharacters[0].inventory.entry(137));
    EXPECT_EQ(joined.party.pCharacters[0].inventory.entry(137)->mm6RawRecord,
              fields.inventory.characters[0].items[137]);
    EXPECT_EQ(mm6::captureRawPartyInventories(joined, *table.table, GameTitle::MM6),
              fields.inventory);
    EXPECT_EQ(baseline.uNumGold, 999);
    EXPECT_FALSE(baseline.pCharacters[0].inventory.entry(137));

    EXPECT_THROW((void) mm6::stageRawPartyCoreProgressInventory(
        baseline, owners, *table.table, GameTitle::MM7), std::exception);
    owners.inventory.characters[3].items[137].setWord(0, 10);
    owners.inventory.characters[3].items[137].setWord(12, 4);
    owners.inventory.characters[3].gridWords[0] = 138;
    EXPECT_THROW((void) mm6::stageRawPartyCoreProgressInventory(
        baseline, owners, *table.table, GameTitle::MM6), std::exception);
    owners.saved.inventory = owners.inventory;
    EXPECT_THROW((void) mm6::stageRawPartyCoreProgressInventory(
        baseline, owners, *table.table, GameTitle::MM6), std::exception);
    EXPECT_EQ(baseline.uNumGold, 999);
    EXPECT_FALSE(baseline.pCharacters[3].inventory.entry(137));
}

UNIT_TEST(MM6RawSaveRuntime, StagesWorldAndVisualOwnersAfterValidatedRawMap) {
    const auto fields = exampleRaw();
    const auto names = namesOf(fields);
    auto raw = mm6::stageRawTitleOwners(mm6::encodeRawSave(fields, GameTitle::MM6),
        names, syntheticBase, validItem, GameTitle::MM6);
    NPCStats liveNpcs;
    Timer liveTimer;
    ActiveOverlayList liveOverlays;
    auto staged = mm6::stageRawWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, raw, 5, GameTitle::MM6);
    EXPECT_EQ(staged.world.mapDeltas.size(), 67);
    EXPECT_EQ(staged.world.npcs.uNumNewNPCs, 399);
    EXPECT_TRUE(staged.visuals.timer.isPaused());
    EXPECT_EQ(staged.rawActiveDelta, fields.chest.rawActiveDelta);
    EXPECT_TRUE(staged.chestTitle.ready);
    EXPECT_EQ(liveNpcs.uNumNewNPCs, 0);
    EXPECT_THROW((void) mm6::stageRawWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, raw, 5, GameTitle::MM7), std::exception);

    raw.saved.metadata.overlays[0].animationLength = 1;
    raw.saved.metadata.overlays[0].packedPid = Pid::actor(0).packed();
    EXPECT_THROW((void) mm6::stageRawWorldAndVisuals(
        liveNpcs, liveTimer, liveOverlays, raw, 5, GameTitle::MM6), std::exception);
    EXPECT_EQ(liveNpcs.uNumNewNPCs, 0);
    EXPECT_FALSE(liveTimer.isPaused());
}

UNIT_TEST(MM6RawSaveRuntime, StagesAllRepresentedPartyFieldsBeforeLateClockFailure) {
    ItemTableScope table;
    auto fields = exampleRaw();
    fields.core.party.gold = 123;
    fields.resistances.characters[1].base[0] = -17;
    fields.buffs.party[0].expireTicks = 100;
    fields.buffs.party[0].power = 3;
    fields.hirelings.displayNames[0] = "Mira";
    fields.recovery.daysWithoutRest = 2;
    const auto names = namesOf(fields);
    auto owners = mm6::stageRawTitleOwners(mm6::encodeRawSave(fields, GameTitle::MM6),
        names, syntheticBase, validItem, GameTitle::MM6);
    Party baseline;
    baseline.uNumGold = 999;
    auto staged = mm6::stageRawPartyRepresentedFields(
        baseline, owners, *table.table, GameTitle::MM6);
    EXPECT_EQ(staged.party.uNumGold, 123);
    EXPECT_EQ(mm6::captureResistanceFields(staged.party, GameTitle::MM6), fields.resistances);
    EXPECT_EQ(mm6::captureBuffFields(staged.party, GameTitle::MM6), fields.buffs);
    EXPECT_EQ(staged.party.pHireling1Name, "Mira");
    EXPECT_EQ(staged.party.days_played_without_rest, 2);
    EXPECT_EQ(mm6::captureRawPartyInventories(staged, *table.table, GameTitle::MM6),
              fields.inventory);

    owners.saved.recovery.playingTicks = 41;
    EXPECT_THROW((void) mm6::stageRawPartyRepresentedFields(
        baseline, owners, *table.table, GameTitle::MM6), std::exception);
    EXPECT_EQ(baseline.uNumGold, 999);
    EXPECT_TRUE(baseline.pHireling1Name.empty());
    EXPECT_FALSE(baseline.pCharacters[0].inventory.entry(137));
}

#ifndef _WIN32
UNIT_TEST(MM6RawSaveRuntime, NativeSlotPreflightStagesSemanticBackupWithoutRepair) {
    TestProfile profile;
    auto good = exampleRaw();
    const auto names = namesOf(good);
    const auto goodBytes = mm6::encodeRawSave(good, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", goodBytes);

    auto bad = good;
    bad.inventory.characters[0].items[137].setWord(0, 581);
    const auto badBytes = mm6::encodeRawSave(bad, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", badBytes);
    auto selected = mm6::preflightStagedRawSaveSlot(
        profile.root(), "save001.ec6", names, syntheticBase, validItem, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->owners.inventory.characters[0].items[137].itemNumberRaw(), 10u);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), goodBytes);

    writeBytes(profile.backup(), badBytes);
    EXPECT_THROW((void) mm6::preflightStagedRawSaveSlot(
        profile.root(), "save001.ec6", names, syntheticBase, validItem, GameTitle::MM6),
        std::exception);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), badBytes);
    EXPECT_THROW((void) mm6::preflightStagedRawSaveSlot(
        profile.root(), "save001.ec6", names, syntheticBase, validItem, GameTitle::MM7),
        std::exception);
    EXPECT_FALSE(mm6::preflightStagedRawSaveSlot(
        profile.root(), "save002.ec6", names, syntheticBase, validItem, GameTitle::MM6));
}

UNIT_TEST(MM6RawSaveRuntime, FullRepresentedPreflightFallsBackAfterLatePartyFailure) {
    ItemTableScope table;
    TestProfile profile;
    const auto good = exampleRaw();
    const auto names = namesOf(good);
    const auto goodBytes = mm6::encodeRawSave(good, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", goodBytes);
    auto bad = good;
    bad.inventory.characters[0].items[137].setWord(12, 4);
    const auto badBytes = mm6::encodeRawSave(bad, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", badBytes);

    const auto freshNpcs = freshNpcBaseline();
    auto selected = mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", freshNpcs,
        names, syntheticBase, validItem, *table.table, 5, 145,
        GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    ASSERT_TRUE(selected->party.party.pCharacters[0].inventory.entry(137));
    EXPECT_EQ(selected->party.party.pCharacters[0].inventory.entry(137)->itemId,
              static_cast<ItemId>(10));
    EXPECT_EQ(selected->world.world.mapDeltas.size(), 67);
    ASSERT_TRUE(selected->indoorPlan);
    EXPECT_EQ(selected->indoorPlan->mapName, "synthetic0.blv");
    EXPECT_FALSE(selected->outdoorPlan);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), goodBytes);

    writeBytes(profile.backup(), badBytes);
    EXPECT_THROW((void) mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", freshNpcs,
        names, syntheticBase, validItem, *table.table, 5, 145,
        GameTitle::MM6), std::exception);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), badBytes);
}

UNIT_TEST(MM6RawSaveRuntime, RawSessionPreflightPreparesOutdoorPlanBeforeSelection) {
    ItemTableScope table;
    TestProfile profile;
    auto indoor = exampleRaw();
    const auto names = namesOf(indoor);
    const auto indoorBytes = mm6::encodeRawSave(indoor, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", indoorBytes);
    auto outdoor = indoor;
    outdoor.metadata.header.mapFileName = "outside0.odm";
    outdoor.chest.activeDeltaName = "outside0.ddm";
    outdoor.chest.rawActiveDelta = outdoorDelta();
    const auto outdoorBytes = mm6::encodeRawSave(outdoor, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", outdoorBytes);

    const auto npcs = freshNpcBaseline();
    // The raw MAPS pass accepts this synthetic base, while the outdoor plan
    // rejects its zero tile selectors. Slot selection must fall back to the
    // indoor backup without repairing either file.
    auto selected = mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", npcs,
        names, syntheticBase, validItem, *table.table, 5, 145, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_TRUE(selected->indoorPlan);
    EXPECT_FALSE(selected->outdoorPlan);
    EXPECT_EQ(readBytes(profile.primary()), outdoorBytes);
    EXPECT_EQ(readBytes(profile.backup()), indoorBytes);

    const auto boundedBase = [](std::string_view name) {
        auto bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
        if (name.ends_with(".odm"))
            for (std::size_t i = 0; i < 4; ++i)
                bytes[160 + i * 4 + 2] = static_cast<std::uint8_t>(1 + i * 36);
        return Blob::copy(bytes.data(), bytes.size());
    };
    selected = mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", npcs,
        names, boundedBase, validItem, *table.table, 5, 145, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_FALSE(selected->needsRecovery);
    EXPECT_FALSE(selected->indoorPlan);
    ASSERT_TRUE(selected->outdoorPlan);
    EXPECT_EQ(selected->outdoorPlan->mapName, "outside0.odm");
    EXPECT_EQ(selected->outdoorPlan->terrainTileIds.size(), 127 * 127);
    EXPECT_EQ(readBytes(profile.primary()), outdoorBytes);
}

UNIT_TEST(MM6RawSaveRuntime, RawSessionPreflightRejectsOpaqueEventBytesBeforeSelection) {
    ItemTableScope table;
    TestProfile profile;
    auto good = exampleRaw();
    constexpr std::size_t persistent = 8 + 875 + 3 * 4 + 200 * 80;
    good.maps.deltas[0].bytes[persistent + 70] = 0x5a;
    const auto names = namesOf(good);
    const auto goodBytes = mm6::encodeRawSave(good, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", goodBytes);
    auto bad = good;
    bad.maps.deltas[0].bytes[persistent + 71] = 1;
    const auto badBytes = mm6::encodeRawSave(bad, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", badBytes);

    const auto npcs = freshNpcBaseline();
    auto selected = mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", npcs,
        names, syntheticBase, validItem, *table.table, 5, 145, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->persistentEvents.mapBytes[70], 0x5a);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), goodBytes);
}

UNIT_TEST(MM6RawSaveRuntime, RawSessionPreflightRejectsActiveActorOverlayBeforeSelection) {
    ItemTableScope table;
    TestProfile profile;
    auto good = exampleRaw();
    const auto names = namesOf(good);
    const auto goodBytes = mm6::encodeRawSave(good, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", goodBytes);
    auto bad = good;
    bad.maps.deltas[0].bytes = indoorDeltaWithActor();
    bad.chest.rawActiveDelta = bad.maps.deltas[0].bytes;
    bad.metadata.overlays[0].animationLength = 1;
    bad.metadata.overlays[0].packedPid = Pid::actor(0).packed();
    const auto badBytes = mm6::encodeRawSave(bad, GameTitle::MM6);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", badBytes);

    const auto npcs = freshNpcBaseline();
    auto selected = mm6::preflightStagedRawSessionFields(
        profile.root(), "save001.ec6", npcs,
        names, syntheticBase, validItem, *table.table, 5, 145, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->raw.saved.metadata.overlays[0].animationLength, 0);
    EXPECT_EQ(readBytes(profile.primary()), badBytes);
    EXPECT_EQ(readBytes(profile.backup()), goodBytes);
}
#endif
