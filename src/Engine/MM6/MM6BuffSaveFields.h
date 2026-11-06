#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;

namespace mm6 {

struct SavedSpellBuff {
    std::int64_t expireTicks = 0; // -1 expired, 0 inactive, positive active.
    std::uint16_t power = 0;
    std::uint8_t mastery = 0;
    std::uint16_t overlayId = 0;
    std::int8_t caster = -1;
    bool isGM = false; // Also used by MM6 NPC flight/water-walk buffs.

    bool operator==(const SavedSpellBuff &) const = default;
};

struct BuffSaveFields {
    std::array<SavedSpellBuff, 20> party;
    std::array<std::array<SavedSpellBuff, 24>, 4> characters;

    bool operator==(const BuffSaveFields &) const = default;
};

SaveSection encodeBuffSection(const BuffSaveFields &fields);
BuffSaveFields decodeBuffSection(const SaveContainer &container);
void loadBuffFieldsBeforeMutation(std::span<const std::uint8_t> bytes, BuffSaveFields *destination);

BuffSaveFields captureBuffFields(const Party &party, GameTitle title);
Party stageBuffParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
