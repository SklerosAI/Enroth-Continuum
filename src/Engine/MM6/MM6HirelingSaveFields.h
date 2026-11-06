#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>

#include "Engine/GameTitle.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

struct SavedSacrificeStatus {
    bool inProgress = false;
    std::int64_t elapsedTicks = 0;
    std::int64_t endTicks = 0;

    bool operator==(const SavedSacrificeStatus &) const = default;
};

struct HirelingSaveFields {
    std::array<NPCData, 2> hirelings;
    std::array<std::string, 2> displayNames;
    std::array<SavedSacrificeStatus, 2> sacrifice;
};

SaveSection encodeHirelingSection(const HirelingSaveFields &fields);
HirelingSaveFields decodeHirelingSection(const SaveContainer &container);
void loadHirelingFieldsBeforeMutation(std::span<const std::uint8_t> bytes, HirelingSaveFields *destination);

HirelingSaveFields captureHirelingFields(const Party &party, GameTitle title);
Party stageHirelingParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
