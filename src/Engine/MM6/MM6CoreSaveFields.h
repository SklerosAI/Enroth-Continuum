#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "MM6SaveContainer.h"

namespace mm6 {

// A deliberately incomplete field family. The full .ec6 save cannot be
// enabled until inventory, conditions, quests, world deltas and other families
// have equally checked codecs.
struct CharacterModifierFields {
    std::array<std::int32_t, 7> statBonuses{};
    std::int16_t ac = 0;
    std::int16_t level = 0;
    std::int16_t age = 0;
    std::int32_t birthYear = 0;
    std::array<bool, 7> pureStatPotionUsed{};

    bool operator==(const CharacterModifierFields &) const = default;
};

struct CoreCharacterFields {
    std::string name;
    std::uint8_t sex = 0;
    std::uint8_t classId = 0;
    std::uint8_t faceId = 0;
    std::uint8_t voiceId = 0;
    std::uint16_t level = 0;
    std::uint64_t experience = 0;
    std::int32_t health = 0;
    std::int32_t mana = 0;
    std::int32_t skillPoints = 0;
    std::array<std::int32_t, 7> baseStats{};
    std::array<std::uint16_t, 31> skills{};
    std::array<bool, 99> spells{};
    CharacterModifierFields modifiers;

    bool operator==(const CoreCharacterFields &) const = default;
};

struct CorePartyFields {
    std::int32_t gold = 0;
    std::int32_t food = 0;
    std::int32_t bankGold = 0;
    std::int64_t playingTicks = 0;
    std::array<float, 3> position{};
    std::int32_t viewYaw = 0;
    std::int32_t viewPitch = 0;
    std::int8_t activeCharacter = -1;

    bool operator==(const CorePartyFields &) const = default;
};

struct CoreSaveFields {
    CorePartyFields party;
    std::array<CoreCharacterFields, 4> characters;

    bool operator==(const CoreSaveFields &) const = default;
};

/** Make the required PART and CHAR sections after validating every field. */
std::array<SaveSection, 2> encodeCoreSections(const CoreSaveFields &fields);

/** Decode both sections into temporary state; no live session is mutated. */
CoreSaveFields decodeCoreSections(const SaveContainer &container);

/** Full container integrity and both field families are checked before assignment. */
void loadCoreFieldsBeforeMutation(std::span<const std::uint8_t> bytes, CoreSaveFields *destination);

} // namespace mm6
