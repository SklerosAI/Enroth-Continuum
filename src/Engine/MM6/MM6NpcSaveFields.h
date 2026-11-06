#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6SaveContainer.h"

namespace mm6 {

inline constexpr size_t MAX_NPC_RECORD_BYTES = 2 + 1024 + 66;

// Shared EC6 logical NPC record used by RESC and the two party hireling copies.
// This is not a native MM6 or MM7 binary layout.
std::vector<std::uint8_t> encodeNpcRecord(const NPCData &npc);
NPCData decodeNpcRecord(std::span<const std::uint8_t> bytes);

// RESC is an EC6 runtime state section, not a native MM6 save record. Every
// NPCData scalar is stored without interpreting profession, event or flag bits.
struct NpcSaveFields {
    std::array<NPCData, 501> npcData;
    std::array<std::uint16_t, 51> groups{};
    int activeCount = 0;
};

SaveSection encodeNpcSection(const NpcSaveFields &fields);
NpcSaveFields decodeNpcSection(const SaveContainer &container);
void loadNpcFieldsBeforeMutation(std::span<const std::uint8_t> bytes, NpcSaveFields *destination);

NpcSaveFields captureNpcFields(const NPCStats &stats, GameTitle title);

// Decode, validate and apply to a copy. Original tables, generated-map NPCs,
// profession metadata and other NPCStats members remain on that copy.
NPCStats stageNpcStats(const NPCStats &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
