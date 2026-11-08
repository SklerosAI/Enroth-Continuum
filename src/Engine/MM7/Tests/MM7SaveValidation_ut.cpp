#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/NPCEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/NPCTable.h"

#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormats.h"

#include "Utility/Exception.h"

namespace {

std::string emptyDelta(bool indoor) {
    const std::size_t prefix = sizeof(LocationHeader_MM7) + (indoor ? 875 : 2 * 88 * 11);
    const std::size_t tail = sizeof(PersistentVariables_MM7) + sizeof(std::int64_t) + sizeof(MapWeather_MM7);
    // Actor, sprite and chest vectors each have a zero count.
    return std::string(prefix + 3 * sizeof(std::uint32_t) + tail, '\0');
}

void putUint32(std::string *data, std::size_t offset, std::uint32_t value) {
    for (std::size_t byte = 0; byte < sizeof(value); ++byte)
        (*data)[offset + byte] = static_cast<char>(value >> (8 * byte));
}

} // namespace

UNIT_TEST(MM7SaveValidation, BoundedLODEntryRejectsDeclaredExpansion) {
    Blob original = Blob::fromString("MM7 bounded decode");
    Blob compressed = lod::encodeCompressed(original);
    Blob decoded = mm7::detail::decodeBoundedSaveEntry(compressed);
    EXPECT_EQ(decoded.str(), original.str());

    std::string corrupted(compressed.str());
    putUint32(&corrupted, 12, mm7::detail::maxDecodedSaveEntryBytes + 1);
    EXPECT_THROW(mm7::detail::decodeBoundedSaveEntry(Blob::fromString(corrupted)), Exception);
}

UNIT_TEST(MM7SaveValidation, IndoorDeltaCountsAreBoundedBeforeDecode) {
    std::string valid = emptyDelta(true);
    EXPECT_TRUE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(valid), true, 0, 0));

    const std::size_t actorCount = sizeof(LocationHeader_MM7) + 875;
    std::string hugeCount = valid;
    putUint32(&hugeCount, actorCount, 0xffffffffu);
    EXPECT_FALSE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(hugeCount), true, 0, 0));

    valid.pop_back();
    EXPECT_FALSE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(valid), true, 0, 0));
    EXPECT_FALSE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(emptyDelta(true)),
                                                     true, 0, 0, 0, 1));
}

UNIT_TEST(MM7SaveValidation, OutdoorDeltaCountsAndTailAreExact) {
    std::string valid = emptyDelta(false);
    EXPECT_TRUE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(valid), false, 0, 0));

    const std::size_t spriteCount = sizeof(LocationHeader_MM7) + 2 * 88 * 11 + sizeof(std::uint32_t);
    std::string oversized = valid;
    putUint32(&oversized, spriteCount, 10);
    EXPECT_FALSE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(oversized), false, 0, 0));

    valid.push_back('\0');
    EXPECT_FALSE(mm7::detail::hasBoundedDeltaRecords(Blob::fromString(valid), false, 0, 0));
}

UNIT_TEST(MM7SaveValidation, ForeignTitleAndClosedGameArchiveRefuse) {
    LodReader closedGamesLod;
    const std::string bytes = "not a save";
    const auto span = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(bytes.data()), bytes.size());
    EXPECT_FALSE(mm7::preflightNativeSaveStructure(span, GameTitle::MM6, closedGamesLod));
    EXPECT_FALSE(mm7::preflightNativeSaveStructure(span, GameTitle::MM7, closedGamesLod));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidCharacterInventoryReferences) {
    Character_MM7 character{};
    EXPECT_TRUE(mm7::detail::hasSafeCharacterInventoryReferences(character));

    character.inventoryMatrix[0] = 127;
    EXPECT_FALSE(mm7::detail::hasSafeCharacterInventoryReferences(character));
    character.inventoryMatrix[0] = 126;
    EXPECT_TRUE(mm7::detail::hasSafeCharacterInventoryReferences(character));

    character.equipment[0] = 127;
    EXPECT_FALSE(mm7::detail::hasSafeCharacterInventoryReferences(character));
    character.equipment[0] = 0;
    character.inventoryItems[125].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeCharacterInventoryReferences(character));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidHeldShopAndGuildItems) {
    Party_MM7 party{};
    EXPECT_TRUE(mm7::detail::hasSafePartyInventoryReferences(party));

    party.pickedItem.itemId = 799;
    party.standartItemsInShops[0][0].itemId = 1;
    party.specialItemsInShops[50][11].itemId = 799;
    party.spellBooksInGuilds[31][11].itemId = 1;
    EXPECT_TRUE(mm7::detail::hasSafePartyInventoryReferences(party));

    party.pickedItem.itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.pickedItem.itemId = 0;
    party.standartItemsInShops[0][0].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.standartItemsInShops[0][0].itemId = 0;
    party.specialItemsInShops[50][11].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.specialItemsInShops[50][11].itemId = 0;
    party.spellBooksInGuilds[31][11].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.spellBooksInGuilds[31][11].itemId = 0;

    party.standartItemsInShops[0][5].itemId = 800; // Last active weapon slot.
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.standartItemsInShops[0][5].itemId = 0;
    party.specialItemsInShops[14][7].itemId = 800; // Last active armor slot.
    EXPECT_FALSE(mm7::detail::hasSafePartyInventoryReferences(party));
    party.specialItemsInShops[14][7].itemId = 0;

    party.standartItemsInShops[0][6].itemId = 800; // Weapon shop tail.
    party.specialItemsInShops[14][8].itemId = 800; // Armor shop tail.
    party.specialItemsInShops[51][11].itemId = 800; // Unused house.
    EXPECT_TRUE(mm7::detail::hasSafePartyInventoryReferences(party));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidPartyAndCharacterEnums) {
    Party_MM7 party{};
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));

    party.alignment = 3;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    party.alignment = 2;
    party.field_7B5_in_arena_quest = -1;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    party.field_7B5_in_arena_quest = 85;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    party.field_7B5_in_arena_quest = 88;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    party.field_7B5_in_arena_quest = 89;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    party.field_7B5_in_arena_quest = 0;

    Character_MM7 &character = party.characters[0];
    character.sex = 2;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.sex = 1;
    character.classType = 36;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.classType = 35;
    character.lastOpenedSpellbookPage = 9;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.lastOpenedSpellbookPage = -1;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.lastOpenedSpellbookPage = 8;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));

    character.quickSpell = 1;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    character.quickSpell = 99;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    character.quickSpell = 100; // Projectile IDs are not selectable quick spells.
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.quickSpell = 0; // SPELL_NONE is the fresh-character default.
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
}

UNIT_TEST(MM7SaveValidation, ChecksOnlyNamedHirelingProfessionBounds) {
    Party_MM7 party{};
    const std::uint32_t lastProfession = std::to_underlying(NPC_PROFESSION_LAST);

    party.hirelings[0].profession = lastProfession + 1;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party)); // Inactive slot retains stale data.

    party.hirelings[0].name.value = 1;
    party.hirelings[0].profession = std::to_underlying(NPC_PROFESSION_NONE);
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    party.hirelings[0].profession = lastProfession;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    party.hirelings[0].profession = lastProfession + 1;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));

    party.hirelings[0].name.value = 0;
    party.hirelings[1].name.value = 1;
    party.hirelings[1].profession = lastProfession + 1;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
}

UNIT_TEST(MM7SaveValidation, RejectsOutOfRangeHiredNpcProfessionWithoutSavedName) {
    std::array<NPCData_MM7, 501> npcData{};
    npcData[3].name.value = 0; // Static NPC names are restored from the table after load.
    npcData[3].flags = std::to_underlying(NPC_HIRED);
    npcData[3].profession = 78;
    EXPECT_FALSE(mm7::detail::hasSafeHiredNpcProfessions(npcData));

    npcData[3].profession = 77;
    EXPECT_TRUE(mm7::detail::hasSafeHiredNpcProfessions(npcData));
    npcData[3].profession = 0;
    EXPECT_TRUE(mm7::detail::hasSafeHiredNpcProfessions(npcData));

    npcData[3].flags = 0;
    npcData[3].profession = 78;
    npcData[500].profession = 0xffffffffu; // Unhired and unreferenced in this fixture.
    EXPECT_TRUE(mm7::detail::hasSafeHiredNpcProfessions(npcData));
}

UNIT_TEST(MM7SaveValidation, AcceptsZombieFacesAndRejectsOutOfRangeFaceOrVoice) {
    Party_MM7 party{};
    Character_MM7 &character = party.characters[0];
    character.currentFace = 23;
    character.voiceId = 24;
    character.prevFace = 24;
    character.prevVoiceId = 23;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));

    character.currentFace = 25;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.currentFace = 24;
    character.voiceId = 25;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.voiceId = 24;
    character.prevFace = -1;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    character.prevFace = 23;
    character.prevVoiceId = 25;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
}

UNIT_TEST(MM7SaveValidation, ChecksOnlyActiveBeaconMapIndices) {
    Party_MM7 party{};
    LloydBeacon_MM7 &beacon = party.characters[0].installedBeacons[0];
    beacon.mapIndexInGamesLod = 0xffff;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));

    beacon.beaconTime = 1;
    beacon.mapIndexInGamesLod = 0;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    beacon.mapIndexInGamesLod = 75;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
    beacon.mapIndexInGamesLod = 76;
    EXPECT_FALSE(mm7::detail::hasSafePartySemanticReferences(party));
    beacon.beaconTime = 0;
    EXPECT_TRUE(mm7::detail::hasSafePartySemanticReferences(party));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidChestTypesItemsAndGridReferences) {
    Chest_MM7 chest{};
    EXPECT_TRUE(mm7::detail::hasSafeChestReferences(chest));

    chest.chestTypeId = 8;
    EXPECT_FALSE(mm7::detail::hasSafeChestReferences(chest));
    chest.chestTypeId = 0;
    chest.inventoryMatrix[0] = 82;
    EXPECT_FALSE(mm7::detail::hasSafeChestReferences(chest));
    chest.inventoryMatrix[0] = 81;
    EXPECT_TRUE(mm7::detail::hasSafeChestReferences(chest));
    chest.inventoryMatrix[81] = 82; // Outside the active 9 x 9 grid.
    EXPECT_TRUE(mm7::detail::hasSafeChestReferences(chest));
    chest.items[139].itemId = 800;
    EXPECT_TRUE(mm7::detail::hasSafeChestReferences(chest)); // Structural check only; live guard pending.
    chest.items[80].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeChestReferences(chest));
    chest.items[80].itemId = 0;
    chest.inventoryMatrix[0] = 82;
    EXPECT_FALSE(mm7::detail::hasSafeChestReferences(chest));
}

UNIT_TEST(MM7SaveValidation, RejectsUnmappedNonMissileSpriteItems) {
    OutdoorDelta_MM7 delta{};
    SpriteObject_MM7 sprite{};
    sprite.containing_item.itemId = 800;
    delta.spriteObjects.push_back(sprite);
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));

    delta.spriteObjects[0].uAttributes = static_cast<std::uint16_t>(SPRITE_MISSILE);
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.spriteObjects[0].uAttributes = 0;
    delta.spriteObjects[0].containing_item.itemId = 799;
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
}

UNIT_TEST(MM7SaveValidation, AcceptsActorReferenceBoundsInBothDeltaKinds) {
    Actor_MM7 actor{};
    actor.monsterInfo.id = 1;
    actor.monsterId = 0; // Scripted actors can have no table monster ID.
    actor.uniqueNameIndex = 0;
    actor.hostilityGroup = 0; // Derive from monster type.

    IndoorDelta_MM7 indoor{};
    OutdoorDelta_MM7 outdoor{};
    indoor.actors.push_back(actor);
    outdoor.actors.push_back(actor);
    EXPECT_TRUE(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0));
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(outdoor));

    indoor.actors[0].monsterId = 1;
    outdoor.actors[0].monsterId = 1;
    EXPECT_TRUE(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0));
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(outdoor));

    indoor.actors[0].monsterInfo.id = 276;
    indoor.actors[0].monsterId = 276;
    indoor.actors[0].uniqueNameIndex = 30;
    indoor.actors[0].hostilityGroup = 92;
    outdoor.actors[0] = indoor.actors[0];
    EXPECT_TRUE(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0));
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(outdoor));

    indoor.actors[0].hostilityGroup = 9999; // Party ally sentinel.
    outdoor.actors[0].hostilityGroup = 9999;
    EXPECT_TRUE(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0));
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(outdoor));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidActorReferenceBounds) {
    OutdoorDelta_MM7 delta{};
    Actor_MM7 actor{};
    actor.monsterInfo.id = 1;
    delta.actors.push_back(actor);
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));

    delta.actors[0].monsterInfo.id = 0;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].monsterInfo.id = 277;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].monsterInfo.id = 1;

    delta.actors[0].monsterId = -1;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].monsterId = 277;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].monsterId = 0;

    delta.actors[0].uniqueNameIndex = -1;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].uniqueNameIndex = 31;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].uniqueNameIndex = 0;

    delta.actors[0].hostilityGroup = 93;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].hostilityGroup = 9998;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].hostilityGroup = 10000;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));

    IndoorDelta_MM7 indoor{};
    delta.actors[0].hostilityGroup = 0;
    delta.actors[0].monsterId = 277;
    indoor.actors = delta.actors;
    EXPECT_FALSE(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0));
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidNonnegativeActorNpcIdsInBothDeltas) {
    Actor_MM7 actor{};
    actor.monsterInfo.id = 1;
    IndoorDelta_MM7 indoor{};
    OutdoorDelta_MM7 outdoor{};
    indoor.actors.push_back(actor);
    outdoor.actors.push_back(actor);

    auto expectBoth = [&](int npcId, bool safe) {
        indoor.actors[0].npcId = npcId;
        outdoor.actors[0].npcId = npcId;
        EXPECT_EQ(mm7::detail::hasSafeIndoorDeltaReferences(indoor, 0, 0, 0), safe) << npcId;
        EXPECT_EQ(mm7::detail::hasSafeOutdoorDeltaReferences(outdoor), safe) << npcId;
    };

    for (int npcId : {-1, 0, 500, 5000, 5099})
        expectBoth(npcId, true);
    for (int npcId : {501, 4999, 5100, 32767})
        expectBoth(npcId, false);
}

UNIT_TEST(MM7SaveValidation, RejectsInvalidActorLootItems) {
    OutdoorDelta_MM7 delta{};
    Actor_MM7 actor{};
    actor.monsterInfo.id = 1;
    actor.carriedItemId = 799;
    actor.items[0].itemId = 1;
    actor.items[1].itemId = 1;
    actor.items[3].itemId = 799;
    delta.actors.push_back(actor);
    EXPECT_TRUE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));

    delta.actors[0].carriedItemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].carriedItemId = 0;
    delta.actors[0].items[0].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].items[0].itemId = 0;
    delta.actors[0].items[1].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
    delta.actors[0].items[1].itemId = 0;
    delta.actors[0].items[3].itemId = 800;
    EXPECT_FALSE(mm7::detail::hasSafeOutdoorDeltaReferences(delta));
}

UNIT_TEST(MM7SaveValidation, RejectsIndoorDoorOverflowAndInvalidIds) {
    IndoorDelta_MM7 delta{};
    BLVDoor_MM7 door{};
    door.numVertices = 1;
    door.numFaces = 1;
    door.numSectors = 1;
    door.numOffsets = 1;
    delta.doors.push_back(door);
    delta.doorsData = {0, 0, 0, 0, 0, 0, 0, 0};
    EXPECT_TRUE(mm7::detail::hasSafeIndoorDeltaReferences(delta, 1, 1, 1));

    delta.doorsData[1] = 1; // Face ID beyond the loaded map.
    EXPECT_FALSE(mm7::detail::hasSafeIndoorDeltaReferences(delta, 1, 1, 1));
    delta.doorsData[1] = 0;
    delta.doors[0].numOffsets = 0;
    EXPECT_FALSE(mm7::detail::hasSafeIndoorDeltaReferences(delta, 1, 1, 1));
    delta.doors[0].numOffsets = 1;
    delta.doorsData.pop_back();
    EXPECT_FALSE(mm7::detail::hasSafeIndoorDeltaReferences(delta, 1, 1, 1));
    delta.doorsData.push_back(0);
    delta.doors.push_back(door); // The second door cannot reuse the first's data.
    EXPECT_FALSE(mm7::detail::hasSafeIndoorDeltaReferences(delta, 1, 1, 1));
}
