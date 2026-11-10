#include "MM6RawSaveRuntime.h"
#include "MM6RawV2Admission.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "MM6AtomicSaveFile.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6DurableSaveFields.h"
#include "MM6HirelingSaveRuntime.h"
#include "MM6RecoverySaveRuntime.h"

namespace mm6 {
namespace {

void validateReferencedItem(const ItemRecord28 &item,
                            const GroundItemVerifier &verifyItem) {
    const std::uint32_t id = item.itemNumberRaw();
    if (id < 1 || id > 580 || !verifyItem(static_cast<std::int32_t>(id)))
        throw std::runtime_error("MM6 raw owner references an invalid item template");
}

void validateRawOwnerItems(const RawSaveFields &save,
                           const GroundItemVerifier &verifyItem) {
    for (const RawCharacterInventory &character : save.inventory.characters) {
        std::array<bool, RAW_INVENTORY_ITEM_SLOTS> referenced{};
        for (std::size_t cell = 0; cell < character.gridWords.size(); ++cell) {
            const std::int32_t word = std::bit_cast<std::int32_t>(character.gridWords[cell]);
            if (word > 0) {
                const std::size_t index = static_cast<std::size_t>(word - 1);
                if (index >= referenced.size() || std::exchange(referenced[index], true))
                    throw std::runtime_error("MM6 raw inventory has a duplicate or out-of-range index");
                validateReferencedItem(character.items[index], verifyItem);
            } else if (word < 0) {
                const std::int64_t anchor = -std::int64_t(word) - 1;
                if (anchor >= static_cast<std::int64_t>(character.gridWords.size()) ||
                    std::bit_cast<std::int32_t>(character.gridWords[anchor]) <= 0)
                    throw std::runtime_error("MM6 raw inventory has an invalid footprint anchor");
            }
        }
        for (std::uint32_t word : character.equipmentWords) {
            if (!word)
                continue;
            const std::size_t index = static_cast<std::size_t>(word - 1);
            if (index >= referenced.size() || std::exchange(referenced[index], true))
                throw std::runtime_error("MM6 raw equipment has a duplicate or out-of-range index");
            validateReferencedItem(character.items[index], verifyItem);
        }
    }
    if (save.heldItem.present)
        validateReferencedItem(save.heldItem.record, verifyItem);
    for (const ShopStockRecord &record : save.shops.stock.records)
        if (record.itemNumberRaw())
            validateReferencedItem(record, verifyItem);
}

} // namespace

StagedRawTitleOwners stageRawTitleOwners(
    std::span<const std::uint8_t> bytes, std::span<const std::string> expectedNames,
    const MapBaseReader &readBase, const GroundItemVerifier &verifyItem,
    GameTitle title) {
    if (title != GameTitle::MM6 || !verifyItem)
        throw std::runtime_error("MM6 raw staging requires a title and item verifier");
    ValidatedRawMapSave validated = decodeRawSaveWithActiveMapCounts(
        bytes, expectedNames, readBase, verifyItem, title);
    validateRawOwnerItems(validated.raw, verifyItem);
    auto cursor = stageRawHeldCursorFromFields(validated.raw.heldItem, title);
    if (!cursor || !validated.raw.inventory.ready || !validated.raw.shops.ready())
        throw std::runtime_error("MM6 raw staging found an unready owner");
    RawInventoryFields inventory = validated.raw.inventory;
    ShopTitleState shops = validated.raw.shops;
    return {std::move(validated.raw), std::move(inventory), std::move(*cursor),
            std::move(shops), validated.active, std::move(validated.activeMap)};
}

RawSaveFields captureRawTitleOwners(const StagedRawTitleOwners &staged,
                                    std::span<const std::string> expectedNames,
                                    const MapBaseReader &readBase,
                                    const GroundItemVerifier &verifyItem,
                                    const Party &sourceParty,
                                    const IndoorFaceOverrides &observedIndoorFaces,
                                    GameTitle title) {
    requireRawV2RepresentableState(sourceParty, observedIndoorFaces, title);
    if (title != GameTitle::MM6 || !verifyItem || !staged.inventory.ready ||
        !staged.shops.ready())
        throw std::runtime_error("MM6 raw capture requires ready title-local owners");
    auto held = captureRawHeldCursor(staged.cursor, title);
    if (!held)
        throw std::runtime_error("MM6 raw capture has an invalid cursor owner");
    RawSaveFields result = staged.saved;
    result.inventory = staged.inventory;
    result.heldItem = std::move(*held);
    result.shops = staged.shops;
    validateRawOwnerItems(result, verifyItem);
    auto validated = decodeRawSaveWithActiveMapCounts(
        encodeRawSave(result, title), expectedNames, readBase, verifyItem, title);
    return std::move(validated.raw);
}

std::optional<StagedRawSaveSlot> preflightStagedRawSaveSlot(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, GameTitle title) {
    if (title != GameTitle::MM6 || !verifyItem)
        throw std::runtime_error("MM6 raw slot preflight requires a title and item verifier");
    std::optional<StagedRawTitleOwners> staged;
    const auto selected = preflightRecoverableRawSaveFile(
        profileRoot, slotName, [&](std::span<const std::uint8_t> bytes) {
            staged = stageRawTitleOwners(bytes, expectedNames, readBase, verifyItem, title);
        });
    if (!selected)
        return std::nullopt;
    if (!staged)
        throw std::runtime_error("MM6 raw slot preflight did not stage selected candidate");
    return StagedRawSaveSlot{std::move(*staged), selected->needsRecovery};
}

StagedRawPartyInventories stageRawPartyCoreProgressInventory(
    const Party &baseline, const StagedRawTitleOwners &owners,
    const ItemTable &itemTable, GameTitle title) {
    requireRawV2NoBeacons(baseline, title);
    if (title != GameTitle::MM6 || owners.saved.inventory != owners.inventory)
        throw std::runtime_error("MM6 raw Party stage has a foreign title or divergent inventory owner");
    Party party = stageCoreParty(baseline, owners.saved.core, title);
    party = stageDurableProgressParty(party, owners.saved.bits, owners.saved.effects, title);
    return stageRawPartyInventories(party, owners.inventory, itemTable, title);
}

StagedRawPartyInventories stageRawPartyRepresentedFields(
    const Party &baseline, const StagedRawTitleOwners &owners,
    const ItemTable &itemTable, GameTitle title) {
    StagedRawPartyInventories staged = stageRawPartyCoreProgressInventory(
        baseline, owners, itemTable, title);
    staged.party = stageResistanceParty(staged.party, owners.saved.resistances, title);
    staged.party = stageBuffParty(staged.party, owners.saved.buffs, title);
    staged.party = stageHirelingParty(staged.party, owners.saved.hirelings, title);
    staged.party = stageRecoveryParty(staged.party, owners.saved.recovery, title);
    return staged;
}

} // namespace mm6
