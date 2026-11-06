#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadHandoff.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {

struct ChestLiveBinding {
    GameTitle title = GameTitle::MM7;
    LevelType levelType = LEVEL_NULL;
    std::vector<Chest> *chests = nullptr;
    std::vector<std::uint8_t> *deltaBytes = nullptr;
    ChestTitleState *titleState = nullptr;
    const MM6IndoorSnapshot *indoorBase = nullptr;  // Required only indoors.
};

struct ChestLiveCandidate {
    std::vector<Chest> chests;
    std::vector<std::uint8_t> deltaBytes;
    std::uint32_t nextLcgState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
    int markersVisited = 0;
};

inline bool copyChestRecords(std::span<const std::uint8_t> section, ChestRecordSet *records) {
    if (!records || section.size() != 4 + 20 * CHEST_WIRE_SIZE)
        return false;
    for (int index = 0; index < 20; ++index)
        std::memcpy((*records)[index].data(), section.data() + 4 + index * CHEST_WIRE_SIZE,
                    CHEST_WIRE_SIZE);
    return true;
}

inline bool liveChestsMatchSidecar(GameTitle title, const std::vector<Chest> &chests,
                                  const ChestRecordSet &records) {
    if (title != GameTitle::MM6 || chests.size() != 20)
        return false;
    for (int index = 0; index < 20; ++index) {
        if (chests[index].position)  // UpdateChestPositions follows the load-time generator call.
            return false;
        Chest_MM7 live{};
        snapshot(chests[index], &live);
        const ChestRecord &raw = records[index];
        if (live.chestTypeId != readChest16(raw, 0) || live.flags != readChest16(raw, 2))
            return false;
        for (int slot = 0; slot < 140; ++slot)
            if (std::memcmp(&live.items[slot], raw.data() + 4 + slot * 28, 24) != 0)
                return false;
        const auto grid = decodeChestGridWire(title, raw);
        if (!grid || live.inventoryMatrix != *grid)
            return false;
    }
    return true;
}

// Parses the current active-map sidecar, checks that the pre-generation live
// vector still matches it, then stages all twenty replacements and a patched
// sidecar. No process or location owner is touched here.
inline std::optional<ChestLiveCandidate> stageChestLiveOwnerHandoff(
    const ChestLiveBinding &binding, MapTreasureLevel mapLevel, const ItemTable &table,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags) {
    if (binding.title != GameTitle::MM6 || !binding.chests || !binding.deltaBytes ||
        !binding.titleState || !binding.titleState->ready || binding.deltaBytes->empty() ||
        (binding.levelType != LEVEL_INDOOR && binding.levelType != LEVEL_OUTDOOR) ||
        (binding.levelType == LEVEL_INDOOR && !binding.indoorBase))
        return std::nullopt;
    try {
        const Blob retained = Blob::copy(binding.deltaBytes->data(), binding.deltaBytes->size());
        ChestMapLoadOwners owners;
        owners.title = binding.title;
        owners.rngState = binding.titleState->lcgState;
        owners.generatedArtifactFlags = binding.titleState->generatedArtifactFlags;
        MM6IndoorSnapshot indoorWire;
        OutdoorDeltaWire outdoorWire;
        if (binding.levelType == LEVEL_INDOOR) {
            indoorWire = deserializeMM6IndoorDelta(retained, *binding.indoorBase);
            const auto &section = indoorWire.section(MM6IndoorSectionKind::Chests);
            if (section.count != 20 || !copyChestRecords(
                    {static_cast<const std::uint8_t *>(section.bytes.data()), section.bytes.size()},
                    &owners.records))
                return std::nullopt;
        } else {
            outdoorWire = decodeOutdoorDelta({static_cast<const std::uint8_t *>(retained.data()), retained.size()});
            if (outdoorWire.chestCount != 20 ||
                !copyChestRecords(outdoorWire.part(OutdoorDeltaPart::Chests), &owners.records))
                return std::nullopt;
        }
        if (!liveChestsMatchSidecar(binding.title, *binding.chests, owners.records))
            return std::nullopt;
        auto staged = stageChestMapLoadHandoff(owners, mapLevel, table, partyOwnedArtifactFlags);
        if (!staged)
            return std::nullopt;

        ChestLiveCandidate result;
        result.chests = std::move(staged->chests);
        result.nextLcgState = staged->nextRngState;
        result.generatedArtifactFlags = staged->generatedArtifactFlags;
        result.markersVisited = staged->markersVisited;
        if (binding.levelType == LEVEL_INDOOR) {
            const auto &section = indoorWire.section(MM6IndoorSectionKind::Chests);
            std::string bytes(static_cast<const char *>(section.bytes.data()), section.bytes.size());
            for (int index = 0; index < 20; ++index)
                std::memcpy(bytes.data() + 4 + index * CHEST_WIRE_SIZE,
                            staged->records[index].data(), CHEST_WIRE_SIZE);
            indoorWire.replaceSection(MM6IndoorSectionKind::Chests, Blob::fromString(std::move(bytes)));
            Blob encoded = serializeMM6Indoor(indoorWire);
            result.deltaBytes.assign(static_cast<const std::uint8_t *>(encoded.data()),
                                     static_cast<const std::uint8_t *>(encoded.data()) + encoded.size());
        } else {
            auto section = outdoorWire.part(OutdoorDeltaPart::Chests);
            for (int index = 0; index < 20; ++index)
                std::memcpy(section.data() + 4 + index * CHEST_WIRE_SIZE,
                            staged->records[index].data(), CHEST_WIRE_SIZE);
            result.deltaBytes = encodeOutdoorDelta(outdoorWire);
        }
        return result;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

static_assert(noexcept(std::declval<std::vector<Chest> &>().swap(std::declval<std::vector<Chest> &>())));
static_assert(noexcept(std::declval<std::vector<std::uint8_t> &>().swap(
    std::declval<std::vector<std::uint8_t> &>())));

inline bool applyChestLiveOwnerHandoff(
    const ChestLiveBinding &binding, MapTreasureLevel mapLevel, const ItemTable &table,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags) {
    auto staged = stageChestLiveOwnerHandoff(binding, mapLevel, table, partyOwnedArtifactFlags);
    if (!staged)
        return false;
    binding.chests->swap(staged->chests);
    binding.deltaBytes->swap(staged->deltaBytes);
    binding.titleState->lcgState = staged->nextLcgState;
    binding.titleState->generatedArtifactFlags = staged->generatedArtifactFlags;
    return true;
}

// Actual engine-owner adapter, deliberately opt-in and not called from the
// guarded load path. The caller must provide title and party-owned artifact
// flags from an MM6-validated source; current SaveLoad does not persist state.
inline bool applyActiveChestLiveOwnerHandoff(
    GameTitle title, MapTreasureLevel mapLevel, const ItemTable &table,
    const std::array<std::uint8_t, 30> &partyOwnedArtifactFlags) {
    if (title != GameTitle::MM6)
        return false;
    ChestLiveBinding binding;
    binding.title = title;
    binding.levelType = uCurrentlyLoadedLevelType;
    binding.chests = &vChests;
    binding.titleState = &chestTitleState();
    if (binding.levelType == LEVEL_OUTDOOR && pOutdoor) {
        binding.deltaBytes = &pOutdoor->mm6DeltaBytes;
    } else if (binding.levelType == LEVEL_INDOOR && pIndoor &&
               pIndoor->doorsData.size() <= std::numeric_limits<std::size_t>::max() / 2) {
        binding.deltaBytes = &pIndoor->mm6DeltaBytes;
        MM6IndoorSnapshot base;
        base.doorDataBytes = pIndoor->doorsData.size() * 2;
        base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
        binding.indoorBase = &base;
        return applyChestLiveOwnerHandoff(binding, mapLevel, table, partyOwnedArtifactFlags);
    } else {
        return false;
    }
    return applyChestLiveOwnerHandoff(binding, mapLevel, table, partyOwnedArtifactFlags);
}

}  // namespace mm6
