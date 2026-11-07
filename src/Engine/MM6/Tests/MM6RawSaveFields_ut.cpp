#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6KnownSaveFields.h"
#include "Engine/MM6/MM6RawSaveFields.h"

namespace {

mm6::RawSaveFields exampleFields() {
    mm6::RawSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.bits.quest[5] = 0x80;
    fields.effects.conditionTicks[3][18] = 900;
    fields.inventory.ready = true;
    fields.inventory.characters[0].items[137].setWord(0, 10);
    fields.inventory.characters[0].items[137][27] = 0xa5;
    fields.inventory.characters[0].gridWords[0] = 138;
    fields.inventory.characters[1].items[17][26] = 0xe1; // Inactive native tail.
    fields.metadata.header.displayName = "Raw candidate";
    fields.metadata.header.mapFileName = "d0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.metadata.ui.activeCharacter = 2;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"d" + std::to_string(i) + ".dlv", {1}});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"o" + std::to_string(i) + ".ddm", {2}});
    fields.chest = {0x12345678, {}, "d0.dlv", {1}};
    fields.npcs.activeCount = 399;
    fields.resistances.characters[0].base[0] = -5;
    fields.heldItem.record[27] = 0x7a; // Empty cursor retains native tail.
    fields.buffs.party[0].power = 12;
    fields.hirelings.displayNames[0] = "Hireling";
    fields.recovery.playingTicks = 42;
    fields.recovery.lastRegenerationTicks = 40;
    fields.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    fields.shops.stock.records[123][27] = 0x5b;
    fields.shops.clocks.deadlineWords[mm6::shopClockSlot(3)] = 72000;
    return fields;
}

mm6::SaveSection &section(mm6::SaveContainer *container, const std::array<char, 4> &tag) {
    auto it = std::find_if(container->sections.begin(), container->sections.end(),
                           [&tag](const mm6::SaveSection &candidate) { return candidate.tag == tag; });
    if (it == container->sections.end())
        throw std::runtime_error("test section absent");
    return *it;
}

} // namespace

UNIT_TEST(MM6RawSaveFields, CoordinatedNineteenSectionRoundTrip) {
    const mm6::RawSaveFields source = exampleFields();
    const auto bytes = mm6::encodeRawSave(source, GameTitle::MM6);
    mm6::SaveContainer container = mm6::decodeSaveContainer(bytes);
    ASSERT_EQ(container.version, 2);
    ASSERT_EQ(container.sections.size(), 19);
    for (const mm6::SaveSection &candidate : container.sections)
        EXPECT_TRUE(candidate.required);
    EXPECT_THROW((void) section(&container, {'I', 'T', 'E', 'M'}), std::exception);
    EXPECT_EQ(section(&container, {'H', 'E', 'L', 'D'}).payload[0], 2);
    EXPECT_EQ(section(&container, {'I', 'N', 'V', 'R'}).payload[0], 1);
    EXPECT_EQ(section(&container, {'S', 'H', 'O', 'P'}).payload[0], 1);

    auto decoded = mm6::decodeRawSave(bytes, GameTitle::MM6);
    EXPECT_EQ(decoded.inventory, source.inventory);
    EXPECT_EQ(decoded.heldItem, source.heldItem);
    EXPECT_EQ(decoded.shops, source.shops);
    EXPECT_EQ(decoded.bits, source.bits);
    EXPECT_EQ(decoded.effects, source.effects);
    EXPECT_EQ(mm6::encodeRawSave(decoded, GameTitle::MM6), bytes);
    EXPECT_THROW((void) mm6::decodeKnownSave(bytes), std::exception);
    EXPECT_THROW((void) mm6::decodeRawSave(bytes, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::encodeRawSave(source, GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6RawSaveFields, OldProvisionalAndMixedShapesReject) {
    mm6::RawSaveFields source = exampleFields();
    mm6::SaveContainer container = mm6::decodeSaveContainer(
        mm6::encodeRawSave(source, GameTitle::MM6));
    mm6::KnownSaveFields old;
    old.core = source.core;
    old.durable.bits = source.bits;
    old.durable.effects = source.effects;
    old.metadata = source.metadata;
    old.maps = source.maps;
    old.npcs = source.npcs;
    old.resistances = source.resistances;
    old.buffs = source.buffs;
    old.hirelings = source.hirelings;
    old.recovery = source.recovery;
    old.chest = source.chest;
    old.shopGuildClocks = source.shops.clocks;
    const auto oldBytes = mm6::encodeKnownSave(old);
    EXPECT_THROW((void) mm6::decodeRawSave(oldBytes, GameTitle::MM6), std::exception);

    mm6::SaveContainer mixed = container;
    mixed.sections.push_back(mm6::encodeDurableSections(old.durable)[2]);
    EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(mixed), GameTitle::MM6),
                 std::exception);
    mixed = container;
    section(&mixed, {'H', 'E', 'L', 'D'}) = mm6::encodeHeldItemSection(old.heldItem);
    EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(mixed), GameTitle::MM6),
                 std::exception);
    mixed = container;
    mixed.version = 1;
    EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(mixed), GameTitle::MM6),
                 std::exception);
}

UNIT_TEST(MM6RawSaveFields, EverySectionMandatoryAndLateFailureLeavesDestinationUntouched) {
    const auto good = mm6::encodeRawSave(exampleFields(), GameTitle::MM6);
    const mm6::SaveContainer baseline = mm6::decodeSaveContainer(good);
    for (std::size_t i = 0; i < baseline.sections.size(); ++i) {
        mm6::SaveContainer missing = baseline;
        missing.sections.erase(missing.sections.begin() + i);
        EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(missing), GameTitle::MM6),
                     std::exception);
        mm6::SaveContainer optional = baseline;
        optional.sections[i].required = false;
        EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(optional), GameTitle::MM6),
                     std::exception);
    }
    mm6::SaveContainer extra = baseline;
    extra.sections.push_back({{'B', 'E', 'A', 'C'}, false, {1}});
    EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(extra), GameTitle::MM6),
                 std::exception);

    mm6::RawSaveFields destination = exampleFields();
    destination.core.party.gold = 777;
    destination.inventory.characters[0].items[137][27] = 0xcc;
    destination.shops.stock.records[123][27] = 0xdd;
    const auto before = mm6::encodeRawSave(destination, GameTitle::MM6);
    constexpr std::array<std::array<char, 4>, 3> lateTags = {{{'S', 'H', 'O', 'P'},
                                                              {'H', 'E', 'L', 'D'},
                                                              {'I', 'N', 'V', 'R'}}};
    for (const std::array<char, 4> &tag : lateTags) {
        mm6::SaveContainer invalid = baseline;
        section(&invalid, tag).payload.pop_back();
        EXPECT_THROW(mm6::loadRawSaveBeforeMutation(
                         mm6::encodeSaveContainer(invalid), &destination, GameTitle::MM6),
                     std::exception);
        EXPECT_EQ(mm6::encodeRawSave(destination, GameTitle::MM6), before);
    }
    EXPECT_THROW(mm6::loadRawSaveBeforeMutation(good, nullptr, GameTitle::MM6), std::exception);
    EXPECT_THROW(mm6::loadRawSaveBeforeMutation(good, &destination, GameTitle::MM7),
                 std::exception);
    EXPECT_EQ(mm6::encodeRawSave(destination, GameTitle::MM6), before);
    mm6::loadRawSaveBeforeMutation(good, &destination, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeRawSave(destination, GameTitle::MM6), good);
}

UNIT_TEST(MM6RawSaveFields, CrossFieldAndOwnerReadinessReject) {
    auto fields = exampleFields();
    fields.metadata.header.playingTicks++;
    EXPECT_THROW((void) mm6::encodeRawSave(fields, GameTitle::MM6), std::exception);
    fields = exampleFields();
    fields.shops.clocks.ready = false;
    EXPECT_THROW((void) mm6::encodeRawSave(fields, GameTitle::MM6), std::exception);
    fields = exampleFields();
    fields.inventory.ready = false;
    EXPECT_THROW((void) mm6::encodeRawSave(fields, GameTitle::MM6), std::exception);

    mm6::SaveContainer container = mm6::decodeSaveContainer(
        mm6::encodeRawSave(exampleFields(), GameTitle::MM6));
    auto changed = exampleFields();
    changed.metadata.header.playingTicks++;
    section(&container, {'H', 'E', 'A', 'D'}) = mm6::encodeMetadataSections(changed.metadata)[0];
    EXPECT_THROW((void) mm6::decodeRawSave(mm6::encodeSaveContainer(container), GameTitle::MM6),
                 std::exception);
}
