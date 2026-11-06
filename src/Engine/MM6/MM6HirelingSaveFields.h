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

struct NpcSaveFields;

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

// An exact same-name/portrait/profession candidate is evidence for a possible
// source record, not proof of identity. Generated NPCs and event mutations can
// legitimately leave a party copy with no hired RESC match.
struct HirelingCorrespondence {
    bool occupied = false;
    std::uint16_t exactCandidates = 0;
    std::uint16_t hiredCandidates = 0;

    bool operator==(const HirelingCorrespondence &) const = default;
};

std::array<HirelingCorrespondence, 2> inspectHirelingCorrespondence(
    const HirelingSaveFields &hirelings, const NpcSaveFields &npcs);

SaveSection encodeHirelingSection(const HirelingSaveFields &fields);
HirelingSaveFields decodeHirelingSection(const SaveContainer &container);
void loadHirelingFieldsBeforeMutation(std::span<const std::uint8_t> bytes, HirelingSaveFields *destination);

HirelingSaveFields captureHirelingFields(const Party &party, GameTitle title);
Party stageHirelingParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
