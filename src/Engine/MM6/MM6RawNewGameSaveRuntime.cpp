#include "MM6RawNewGameSaveRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Objects/Inventory.h"
#include "MM6Archive.h"
#include "MM6AtomicSaveFile.h"
#include "MM6BoundedItemProjection.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6RawV2Admission.h"

namespace mm6 {
namespace {

Party placedParty(const Party &source) {
    Party placed = source;
    placed.pos = Vec3f(-10178, -11206, 340);
    placed.uFallStartZ = 340;
    placed._viewPitch = 0;
    placed._viewYaw = 128;
    return placed;
}

bool hasItemPayload(const Item &item) {
    return item.itemId != ITEM_NULL || item.potionPower || item.goldAmount ||
           item.standardEnchantment || item.standardEnchantmentStrength ||
           item.specialEnchantment != ITEM_ENCHANTMENT_NULL || item.numCharges ||
           item.maxCharges || item.flags || item.lichJarCharacterIndex != -1 ||
           item.enchantmentExpirationTime.ticks();
}

void requireNoInheritedCommerce(const Party &party) {
    for (HouseId id : party.standartItemsInShops.indices()) {
        if (party.PartyTimes.shopNextRefreshTime[id].ticks() ||
            party.PartyTimes.shopBanTimes[id].ticks() || party.InTheShopFlags[id])
            throw std::runtime_error("MM6 raw new-game Party has inherited shop state");
        for (const Item &item : party.standartItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 raw new-game Party has inherited shop stock");
        for (const Item &item : party.specialItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 raw new-game Party has inherited special stock");
    }
    for (HouseId id : party.spellBooksInGuilds.indices()) {
        if (party.PartyTimes.guildNextRefreshTime[id].ticks())
            throw std::runtime_error("MM6 raw new-game Party has inherited guild state");
        for (const Item &item : party.spellBooksInGuilds[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 raw new-game Party has inherited guild stock");
    }
    for (HouseId id : party.PartyTimes.bountyHuntNextGenTime.indices())
        if (party.PartyTimes.bountyHuntNextGenTime[id].ticks() ||
            party.monster_id_for_hunting[id] != MONSTER_INVALID ||
            party.monster_for_hunting_killed[id])
            throw std::runtime_error("MM6 raw new-game Party has inherited bounty state");
}

void requireSourceMatchesSeed(const KnownSaveFields &seed, const Party &party,
                              const IndoorFaceOverrides &observedIndoorFaces,
                              GameTitle title) {
    requireRawV2RepresentableState(party, observedIndoorFaces, title);
    requireNoInheritedCommerce(party);
    const Party placed = placedParty(party);
    const DurableSaveFields progress = captureDurableFields(placed, title);
    if (seed.core != captureCoreFields(placed, title) ||
        seed.durable.bits != progress.bits || seed.durable.effects != progress.effects ||
        seed.resistances != captureResistanceFields(placed, title) ||
        seed.buffs != captureBuffFields(placed, title) ||
        seed.recovery != captureRecoveryFields(placed, title) ||
        encodeHirelingSection(seed.hirelings).payload !=
            encodeHirelingSection(captureHirelingFields(placed, title)).payload ||
        seed.heldItem != captureHeldItem(placed, title))
        throw std::runtime_error("MM6 raw new-game seed differs from source Party");
}

Party normalizeInventoryCapacity(const Party &source) {
    Party normalized = source;
    for (Character &character : normalized.pCharacters) {
        if (character.inventory.capacity() == RAW_INVENTORY_ITEM_SLOTS)
            continue;
        CharacterInventory expanded = CharacterInventory::forMM6();
        for (InventoryConstEntry entry : character.inventory.entries()) {
            if (entry.index() < 0 || entry.index() >= RAW_INVENTORY_ITEM_SLOTS)
                throw std::runtime_error("MM6 new-game item index exceeds raw capacity");
            bool restored = false;
            if (entry.zone() == INVENTORY_ZONE_GRID)
                restored = static_cast<bool>(expanded.tryRestoreAt(entry.index(),
                    Pointi(entry.geometry().x, entry.geometry().y), *entry));
            else if (entry.zone() == INVENTORY_ZONE_EQUIPMENT)
                restored = static_cast<bool>(expanded.tryRestoreAt(entry.index(), entry.slot(), *entry));
            if (!restored)
                throw std::runtime_error("MM6 new-game item cannot retain its physical index");
        }
        character.inventory = std::move(expanded);
    }
    return normalized;
}

void requireRawItemsMatchParty(const Party &party, const RawInventoryFields &inventory,
                               const RawHeldCursorOwner &cursor, const ItemTable &table,
                               GameTitle title) {
    if (title != GameTitle::MM6 || !inventory.ready ||
        !validRawHeldCursorOwner(cursor, title))
        throw std::runtime_error("MM6 raw new-game item owners are not ready");
    Party normalized = normalizeInventoryCapacity(party);
    StagedRawPartyInventories candidate{std::move(normalized), inventory};
    if (captureRawPartyInventories(candidate, table, title) != inventory)
        throw std::runtime_error("MM6 raw new-game inventory differs from source Party");

    const auto held = captureRawHeldCursor(cursor, title);
    if (!held)
        throw std::runtime_error("MM6 raw new-game cursor is invalid");
    if (!held->present) {
        if (hasItemPayload(party.pPickedItem))
            throw std::runtime_error("MM6 raw new-game cursor differs from source Party");
    } else {
        auto projected = stageBoundedItem(held->record, table, title);
        if (!projected)
            throw std::runtime_error("MM6 raw new-game cursor has unsupported item semantics");
        setBoundedItem(*projected, party.pPickedItem);
        const auto replay = captureBoundedItem(*projected, table, title);
        if (!replay || *replay != held->record)
            throw std::runtime_error("MM6 raw new-game cursor differs from source Party");
    }
}

void fillDefaultMapDeltas(RawSaveFields *candidate,
                          std::span<const std::string> expectedNames,
                          const DefaultDeltaReader &readDefaultDelta) {
    candidate->maps.deltas.clear();
    candidate->maps.deltas.reserve(expectedNames.size());
    for (const std::string &name : expectedNames) {
        Blob entry = readDefaultDelta(name);
        if (entry.empty())
            throw std::runtime_error("MM6 raw new-game archive has an empty default delta");
        const auto *start = static_cast<const std::uint8_t *>(entry.data());
        candidate->maps.deltas.push_back({name, {start, start + entry.size()}});
    }
    const auto active = std::find_if(candidate->maps.deltas.begin(),
        candidate->maps.deltas.end(), [](const SavedMapDelta &delta) {
            return delta.name == "oute3.ddm";
        });
    if (active == candidate->maps.deltas.end())
        throw std::runtime_error("MM6 raw new-game archive lacks first-map delta");
    Blob decoded = decodeMM6ArchiveEntry(
        Blob::view(active->bytes.data(), active->bytes.size()));
    const auto *start = static_cast<const std::uint8_t *>(decoded.data());
    candidate->chest.rawActiveDelta.assign(start, start + decoded.size());
}

} // namespace

PreparedRawNewGameAutosave prepareOfflineRawNewGameAutosave(
    const KnownSaveFields &seed, const Party &sourceParty,
    const RawInventoryFields &rawInventory, const RawHeldCursorOwner &cursor,
    const ShopTitleState &shops, const IndoorFaceOverrides &observedIndoorFaces,
    std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefaultDelta, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    GameTitle title) {
    if (title != GameTitle::MM6 || seed.metadata.header.mapFileName != NEW_GAME_FIRST_MAP ||
        seed.chest.activeDeltaName != "oute3.ddm" || expectedNames.size() != 67 ||
        !readDefaultDelta || !readBase || !verifyItem || !shops.ready() ||
        shops != emptyNewGameShopTitleState(title) ||
        seed.shopGuildClocks != shops.clocks)
        throw std::runtime_error("MM6 raw new-game candidate lacks fresh title owners");
    const auto partyArtifacts = scanPartyOwnedChestArtifacts(title, sourceParty);
    if (!partyArtifacts ||
        std::any_of(partyArtifacts->begin(), partyArtifacts->end(),
                    [](std::uint8_t flag) { return flag != 0; }) ||
        std::any_of(seed.chest.generatedArtifactFlags.begin(),
                    seed.chest.generatedArtifactFlags.end(),
                    [](std::uint8_t flag) { return flag != 0; }))
        throw std::runtime_error("MM6 raw new-game candidate cannot retain prior artifacts");
    requireSourceMatchesSeed(seed, sourceParty, observedIndoorFaces, title);
    requireRawItemsMatchParty(sourceParty, rawInventory, cursor, itemTable, title);

    RawSaveFields candidate;
    candidate.core = seed.core;
    candidate.bits = seed.durable.bits;
    candidate.effects = seed.durable.effects;
    candidate.inventory = rawInventory;
    candidate.metadata = seed.metadata;
    candidate.npcs = seed.npcs;
    candidate.resistances = seed.resistances;
    candidate.buffs = seed.buffs;
    candidate.hirelings = seed.hirelings;
    candidate.recovery = seed.recovery;
    candidate.chest = seed.chest;
    candidate.shops = shops;
    const auto held = captureRawHeldCursor(cursor, title);
    if (!held)
        throw std::runtime_error("MM6 raw new-game cursor cannot be captured");
    candidate.heldItem = *held;
    fillDefaultMapDeltas(&candidate, expectedNames, readDefaultDelta);

    std::vector<std::uint8_t> bytes = encodeRawSave(candidate, title);
    StagedRawTitleOwners staged = stageRawTitleOwners(
        bytes, expectedNames, readBase, verifyItem, title);
    if (staged.saved.inventory != rawInventory || staged.saved.heldItem != *held ||
        staged.saved.shops != shops)
        throw std::runtime_error("MM6 raw new-game owners changed during preflight");
    return {std::move(bytes), std::move(staged)};
}

void writeOfflineRawNewGameAutosave(const std::filesystem::path &profileRoot,
                                     const PreparedRawNewGameAutosave &prepared,
                                     GameTitle title) {
    if (title != GameTitle::MM6 || prepared.bytes.empty())
        throw std::runtime_error("MM6 raw new-game write requires prepared v2 bytes");
    // Recheck the exact candidate before the native v2 atomic boundary.
    if (encodeRawSave(prepared.staged.saved, title) != prepared.bytes)
        throw std::runtime_error("MM6 raw new-game prepared candidate changed before write");
    writeAtomicRawSaveFile(profileRoot, NEW_GAME_AUTOSAVE_SLOT, prepared.bytes);
}

} // namespace mm6
