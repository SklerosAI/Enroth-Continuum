#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ChestOpenOwnerTransaction.h"
#include "Engine/MM6/MM6ChestRerollSection.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Tables/MapTable.h"

namespace mm6 {

inline ChestRerollLayouts &chestRerollLayouts() {
    static ChestRerollLayouts owner;
    return owner;
}

/**
 * @param record                        Chest record before its random loot is rolled.
 * @return                              The chest's layout, or `std::nullopt` for a chest without random loot.
 */
inline std::optional<ChestRerollLayout> chestRerollLayout(const ChestRecord &record) {
    ChestRerollLayout layout;
    bool marker = false;
    for (int slot = 0; slot < 140; ++slot) {
        const int id = readChest32(std::span(record).subspan(4 + slot * 28, 28), 0);
        layout[slot] = static_cast<std::int8_t>(id < 0 ? id : id > 0);
        marker |= id < 0;
    }
    return marker ? std::optional(layout) : std::nullopt;
}

/**
 * Keeps the layout of every chest of a map whose random loot was just rolled, so that the chest reroll mod can roll
 * it again. With the mod off it forgets them, and their earlier rolls with them. Chests without random loot keep what
 * they had, a map that comes back rolls nothing.
 *
 * @param[in,out] layouts               Rerollable chests.
 * @param mapName                       Canonical file name of the map.
 * @param before                        The map's chest records before the roll.
 * @param enabled                       Whether the chest reroll mod is on.
 */
inline void noteRolledChests(ChestRerollLayouts *layouts, std::string_view mapName, const ChestRecordSet &before,
                             bool enabled) {
    for (int index = 0; index < 20; ++index) {
        const auto layout = chestRerollLayout(before[index]);
        if (!layout)
            continue;
        if (enabled) {
            (*layouts)[{std::string(mapName), index}] = *layout;
        } else {
            layouts->erase({std::string(mapName), index});
        }
    }
}

/**
 * Runs `noteRolledChests` for the loaded map, with the mod on when `ENROTH_MM6_CHEST_REROLL` turns it on.
 *
 * @param rolledFrom                    The map's chest records before a roll, or `std::nullopt` if nothing was rolled.
 */
inline void noteActiveMapRolledChests(const std::optional<ChestRecordSet> &rolledFrom) {
    if (rolledFrom)
        noteRolledChests(&chestRerollLayouts(), pMapTable->pInfos[engine->_currentLoadedMapId].fileName, *rolledFrom,
                         mods::chestRerollEnabled(engine->gameTitle()));
}

struct ChestRerollCandidate {
    std::vector<Chest> chests;
    std::vector<std::uint8_t> deltaBytes;
    std::uint32_t nextLcgState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

/**
 * Stages a new roll of a chest's random loot against the decoded active sidecar. The slots that held random-loot
 * markers or were empty go back to their layout, and the ordinary MM6 chest generator rolls them again with the chest
 * random state and the map's treasure level. A chest that was placed is placed again. The chest type, flags, trap and
 * fixed items stay.
 *
 * @param binding                       Live chests, sidecar, chest title state, overflow policy and All Artifacts
 *                                      ledger.
 * @param chestIndex                    Chest to reroll.
 * @param layout                        The chest's layout before its first roll.
 * @param mapLevel                      Treasure level of the map.
 * @param table                         Item table.
 * @param dchest                        Contents of `dchest.bin`.
 * @param iconLookup                    Item icon sizes, for the placement.
 * @param partyOwnedArtifactFlags       Artifacts the party carries.
 * @param releaseArtifacts              Whether an artifact of the discarded roll comes off the generated list.
 * @return                              The chests, sidecar, chest random state and generated artifacts after the
 *                                      reroll, or empty if anything refuses.
 */
inline std::optional<ChestRerollCandidate> stageChestReroll(
    const ChestLiveBinding &binding, int chestIndex, const ChestRerollLayout &layout, MapTreasureLevel mapLevel,
    const ItemTable &table, std::span<const std::uint8_t> dchest, const ChestIconLookup &iconLookup,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags, bool releaseArtifacts) {
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
                    {static_cast<const std::uint8_t *>(section.bytes.data()), section.bytes.size()}, &records))
                return std::nullopt;
        } else {
            outdoorWire = decodeOutdoorDelta({static_cast<const std::uint8_t *>(retained.data()), retained.size()});
            if (outdoorWire.chestCount != 20 || !copyChestRecords(outdoorWire.part(OutdoorDeltaPart::Chests), &records))
                return std::nullopt;
        }
        if (!chestMatchesOpenRecord((*binding.chests)[chestIndex], records[chestIndex]))
            return std::nullopt;

        ChestRerollCandidate result;
        result.generatedArtifactFlags = binding.titleState->generatedArtifactFlags;
        ChestRecord record = records[chestIndex];
        for (int slot = 0; slot < 140; ++slot) {
            const auto item = std::span(record).subspan(4 + slot * 28, 28);
            const int id = readChest32(item, 0);
            if (layout[slot] == 1) {
                if (id <= 0)
                    return std::nullopt;
                continue;
            }
            if (releaseArtifacts && id >= 400 && id <= 429)
                result.generatedArtifactFlags[id - 400] = 0;
            std::fill(item.begin(), item.end(), 0);
            writeChest32(item, 0, layout[slot]);
        }
        const bool placed = readChest16(record, 2) & 2u;
        writeChest16(record, 2, static_cast<std::uint16_t>(readChest16(record, 2) & ~2u));
        const auto cleared = encodeChestGridWire(GameTitle::MM6, record, {});
        if (!cleared)
            return std::nullopt;
        record = *cleared;

        ChestRecordGenerationInput input;
        input.title = GameTitle::MM6;
        input.mapTreasureLevel = mapLevel;
        input.rngState = binding.titleState->lcgState;
        input.records[chestIndex] = record;
        input.partyOwnedArtifactFlags = partyOwnedArtifactFlags;
        input.generatedArtifactFlags = result.generatedArtifactFlags;
        if (binding.collectedArtifactFlags)
            input.collectedArtifactFlags = *binding.collectedArtifactFlags;
        const auto generated = stageChestRecordGeneration(input, table);
        if (!generated)
            return std::nullopt;
        record = generated->records[chestIndex];
        result.nextLcgState = generated->nextRngState;
        result.generatedArtifactFlags = generated->generatedArtifactFlags;
        if (placed) {
            const auto placement = stageFirstChestRecordPlacement(GameTitle::MM6, record, dchest, result.nextLcgState,
                                                                  iconLookup, binding.overflow);
            if (!placement)
                return std::nullopt;
            record = placement->record;
            unmarkPlacedChestArtifacts(record);
            result.nextLcgState = placement->nextRngState;
        }

        auto projection = stageChestRuntimeProjection(GameTitle::MM6, record, chestIndex);
        if (!projection)
            return std::nullopt;
        projection->chest.position = (*binding.chests)[chestIndex].position;
        result.chests = *binding.chests;
        result.chests[chestIndex] = std::move(projection->chest);
        if (binding.levelType == LEVEL_INDOOR) {
            const auto &section = indoorWire.section(MM6IndoorSectionKind::Chests);
            std::string bytes(static_cast<const char *>(section.bytes.data()), section.bytes.size());
            std::memcpy(bytes.data() + 4 + chestIndex * CHEST_WIRE_SIZE, record.data(), CHEST_WIRE_SIZE);
            indoorWire.replaceSection(MM6IndoorSectionKind::Chests, Blob::fromString(std::move(bytes)));
            Blob encoded = serializeMM6Indoor(indoorWire);
            result.deltaBytes.assign(static_cast<const std::uint8_t *>(encoded.data()),
                                     static_cast<const std::uint8_t *>(encoded.data()) + encoded.size());
        } else {
            auto section = outdoorWire.part(OutdoorDeltaPart::Chests);
            std::memcpy(section.data() + 4 + chestIndex * CHEST_WIRE_SIZE, record.data(), CHEST_WIRE_SIZE);
            result.deltaBytes = encodeOutdoorDelta(outdoorWire);
        }
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

}  // namespace mm6
