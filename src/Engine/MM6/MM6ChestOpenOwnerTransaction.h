#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestFirstOpenTransaction.h"
#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ChestTrapDecision.h"

namespace mm6 {

/**
 * What the original MM6 chest opening at 0x41e4f0 reads to decide on a trapped chest.
 */
struct ChestTrapCheck {
    bool activeCharacter = false;
    std::optional<int> mapDifficulty; // Lock column of the map, empty for a map missing from the map table.
    int disarmScore = 0; // Active character's score from `stageChestDisarmScore`.
    bool clickedObject = false; // Whether the chest was clicked through a decoration or a face.
};

struct ChestOpenOwnerCandidate {
    std::vector<Chest> chests;
    std::vector<std::uint8_t> deltaBytes;
    std::uint32_t nextLcgState = 0;
    bool placed = false;
    Item heldItem; // The item taken out of the chest, for `ChestInteractionAction::TakeGridItem`.
    std::optional<ChestTrapDecision> trap; // The disarm attempt, if the opening made one.
    bool trapFired = false; // The trap went off instead of the chest opening.
};

// The live chest can have a world position after load. Everything else must
// still agree with the authoritative raw map record before first placement.
inline bool chestMatchesOpenRecord(const Chest &chest, const ChestRecord &record) {
    Chest_MM7 live{};
    snapshot(chest, &live);
    if (live.chestTypeId != readChest16(record, 0) || live.flags != readChest16(record, 2))
        return false;
    const auto grid = decodeChestGridWire(GameTitle::MM6, record);
    if (!grid || live.inventoryMatrix != *grid)
        return false;
    for (int slot = 0; slot < 140; ++slot)
        if (std::memcmp(&live.items[slot], record.data() + 4 + slot * 28, 24) != 0)
            return false;
    return true;
}

/**
 * Stages opening one MM6 chest, or taking a non-gold item out of its grid, against the decoded active sidecar. No GUI,
 * trap effect or inherited MM7 generator is invoked.
 *
 * Opening places the items first. A trapped chest then makes the active character's disarm attempt. A disarmed trap
 * is gone and the chest opens. A trap that goes off is gone too, but the chest stays shut. When the chest wasn't
 * clicked through a decoration or a face, a trap that would go off stays armed and the chest opens.
 *
 * @param binding                       Live chests, sidecar and chest title state.
 * @param chestIndex                    Chest to open.
 * @param dchest                        Contents of `dchest.bin`.
 * @param iconLookup                    Item icon sizes, for the first placement.
 * @param action                        Whether to only open the chest or also take an item.
 * @param gridPoint                     Grid cell of the item to take.
 * @param trapCheck                     Disarm inputs for opening a trapped chest. Without them a trapped chest
 *                                      refuses to open.
 * @return                              The chests, sidecar, chest RNG, taken item and trap outcome after the action,
 *                                      or empty if anything refuses.
 */
inline std::optional<ChestOpenOwnerCandidate> stageChestOpenOwner(
    const ChestLiveBinding &binding, int chestIndex, std::span<const std::uint8_t> dchest,
    const ChestIconLookup &iconLookup, ChestInteractionAction action = ChestInteractionAction::OpenOnly,
    Pointi gridPoint = {}, const std::optional<ChestTrapCheck> &trapCheck = std::nullopt) {
    if (binding.title != GameTitle::MM6 || !binding.chests || binding.chests->size() != 20 ||
        !binding.deltaBytes || binding.deltaBytes->empty() || !binding.titleState ||
        !binding.titleState->ready || chestIndex < 0 || chestIndex >= 20 ||
        (binding.levelType != LEVEL_INDOOR && binding.levelType != LEVEL_OUTDOOR) ||
        (binding.levelType == LEVEL_INDOOR && !binding.indoorBase))
        return std::nullopt;
    try {
        const Blob retained = Blob::copy(binding.deltaBytes->data(), binding.deltaBytes->size());
        MM6IndoorSnapshot indoorWire;
        OutdoorDeltaWire outdoorWire;
        ChestRecordSet records{};
        if (binding.levelType == LEVEL_INDOOR) {
            indoorWire = deserializeMM6IndoorDelta(retained, *binding.indoorBase);
            const auto &section = indoorWire.section(MM6IndoorSectionKind::Chests);
            if (section.count != 20 || !copyChestRecords(
                    {static_cast<const std::uint8_t *>(section.bytes.data()), section.bytes.size()},
                    &records))
                return std::nullopt;
        } else {
            outdoorWire = decodeOutdoorDelta(
                {static_cast<const std::uint8_t *>(retained.data()), retained.size()});
            if (outdoorWire.chestCount != 20 ||
                !copyChestRecords(outdoorWire.part(OutdoorDeltaPart::Chests), &records))
                return std::nullopt;
        }
        if (!chestMatchesOpenRecord((*binding.chests)[chestIndex], records[chestIndex]))
            return std::nullopt;
        ChestFirstOpenInput input;
        input.title = GameTitle::MM6;
        input.record = records[chestIndex];
        input.chestIndex = chestIndex;
        input.dchest = dchest;
        input.lcgState = binding.titleState->lcgState;
        input.iconLookup = iconLookup;
        input.action = action;
        input.gridPoint = gridPoint;
        const auto opened = stageChestFirstOpenInteraction(input);
        if (!opened || opened->itemTaken != (action == ChestInteractionAction::TakeGridItem))
            return std::nullopt;

        ChestOpenOwnerCandidate result;
        ChestRecord record = opened->record;
        result.nextLcgState = opened->nextLcgState;
        if (action == ChestInteractionAction::OpenOnly && (readChest16(record, 2) & 1u)) {
            if (!trapCheck)
                return std::nullopt;
            if (trapCheck->activeCharacter && trapCheck->mapDifficulty) {
                result.trap = stageChestTrapDecision(GameTitle::MM6, record, true, trapCheck->disarmScore,
                                                     *trapCheck->mapDifficulty, result.nextLcgState);
                if (!result.trap)
                    return std::nullopt;
                result.nextLcgState = result.trap->nextLcgState;
                result.trapFired = !result.trap->disarmed && trapCheck->clickedObject;
                if (result.trap->disarmed || result.trapFired)
                    writeChest16(record, 2, static_cast<std::uint16_t>(readChest16(record, 2) & ~1u));
            }
        }

        auto projection = stageChestRuntimeProjection(GameTitle::MM6, record, chestIndex);
        if (!projection)
            return std::nullopt;
        projection->chest.position = (*binding.chests)[chestIndex].position;

        result.chests = *binding.chests;
        result.chests[chestIndex] = std::move(projection->chest);
        result.placed = opened->placed;
        result.heldItem = opened->heldItem;
        if (binding.levelType == LEVEL_INDOOR) {
            const auto &section = indoorWire.section(MM6IndoorSectionKind::Chests);
            std::string bytes(static_cast<const char *>(section.bytes.data()), section.bytes.size());
            std::memcpy(bytes.data() + 4 + chestIndex * CHEST_WIRE_SIZE,
                        record.data(), CHEST_WIRE_SIZE);
            indoorWire.replaceSection(MM6IndoorSectionKind::Chests, Blob::fromString(std::move(bytes)));
            Blob encoded = serializeMM6Indoor(indoorWire);
            result.deltaBytes.assign(static_cast<const std::uint8_t *>(encoded.data()),
                                     static_cast<const std::uint8_t *>(encoded.data()) + encoded.size());
        } else {
            auto section = outdoorWire.part(OutdoorDeltaPart::Chests);
            std::memcpy(section.data() + 4 + chestIndex * CHEST_WIRE_SIZE,
                        record.data(), CHEST_WIRE_SIZE);
            result.deltaBytes = encodeOutdoorDelta(outdoorWire);
        }
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

inline bool applyChestOpenOwner(const ChestLiveBinding &binding, int chestIndex,
                                std::span<const std::uint8_t> dchest,
                                const ChestIconLookup &iconLookup) {
    auto staged = stageChestOpenOwner(binding, chestIndex, dchest, iconLookup);
    if (!staged)
        return false;
    binding.chests->swap(staged->chests);
    binding.deltaBytes->swap(staged->deltaBytes);
    binding.titleState->lcgState = staged->nextLcgState;
    return true;
}

}  // namespace mm6
