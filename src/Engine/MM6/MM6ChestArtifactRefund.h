#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ChestOpenOwnerTransaction.h"

namespace mm6 {

struct ChestArtifactRefundCandidate {
    std::vector<Chest> chests;
    std::vector<std::uint8_t> deltaBytes;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

/**
 * Stages the GrayFace patch's event command `RefundChestArtifacts` against the decoded active sidecar: every artifact
 * that one chest's generation put on the found list, and that no one saw since, comes off the list. The artifacts
 * stay in the chest.
 *
 * @param binding                       Live chests, sidecar and chest title state.
 * @param chestIndex                    Chest to refund.
 * @return                              The chests, sidecar and found artifacts after the refund, or empty if the chest
 *                                      has nothing to refund or anything refuses.
 */
inline std::optional<ChestArtifactRefundCandidate> stageChestArtifactRefund(const ChestLiveBinding &binding, int chestIndex) {
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

        ChestArtifactRefundCandidate result;
        result.generatedArtifactFlags = binding.titleState->generatedArtifactFlags;
        ChestRecord record = records[chestIndex];
        if (!refundChestRecordArtifacts(record, &result.generatedArtifactFlags))
            return std::nullopt;
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

/**
 * Runs `stageChestArtifactRefund` on the loaded map's chests and publishes the result.
 *
 * @param chestIndex                    Chest to refund.
 * @return                              Whether any artifact came off the found list.
 */
inline bool refundActiveChestArtifacts(int chestIndex) {
    ChestLiveBinding binding;
    binding.title = GameTitle::MM6;
    binding.levelType = uCurrentlyLoadedLevelType;
    binding.chests = &vChests;
    binding.titleState = &chestTitleState();
    MM6IndoorSnapshot indoorBase;
    if (binding.levelType == LEVEL_OUTDOOR && pOutdoor) {
        binding.deltaBytes = &pOutdoor->mm6DeltaBytes;
    } else if (binding.levelType == LEVEL_INDOOR && pIndoor &&
               pIndoor->doorsData.size() <= std::numeric_limits<std::size_t>::max() / 2) {
        binding.deltaBytes = &pIndoor->mm6DeltaBytes;
        indoorBase.doorDataBytes = pIndoor->doorsData.size() * 2;
        indoorBase.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
        binding.indoorBase = &indoorBase;
    } else {
        return false;
    }
    auto staged = stageChestArtifactRefund(binding, chestIndex);
    if (!staged)
        return false;
    binding.chests->swap(staged->chests);
    binding.deltaBytes->swap(staged->deltaBytes);
    binding.titleState->generatedArtifactFlags = staged->generatedArtifactFlags;
    return true;
}

}  // namespace mm6
