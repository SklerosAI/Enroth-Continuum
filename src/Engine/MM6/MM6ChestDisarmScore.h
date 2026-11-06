#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "Engine/GameTitle.h"

namespace mm6 {

// Values read by the original MM6 chest disarm-score helper. These booleans
// must come from a title-validated character/equipment and hireling snapshot;
// this detached stage does not scan live MM7-shaped owners.
struct ChestDisarmScoreInput {
    GameTitle title = GameTitle::MM7;
    std::uint8_t encodedSkill = 0;  // Low six bits: level; bit 6: expert; bit 7: master.
    bool unbrokenItem410 = false;
    bool unbrokenItem415 = false;
    bool unbrokenThieveryEnchantment = false;
    bool hasTinker = false;     // Profession 25.
    bool hasLocksmith = false;  // Profession 26.
    bool hasBurglar = false;    // Profession 51.
};

struct ChestDisarmEquippedItem {
    int itemId = 0;
    int specialEnchantment = 0;
    bool broken = false;
};

struct ChestDisarmEquipmentFlags {
    bool unbrokenItem410 = false;
    bool unbrokenItem415 = false;
    bool unbrokenThieveryEnchantment = false;
};

// The original reads item 410 from one fixed slot, item 415 from either of
// two fixed slots, then stops its 16-slot enchantment scan at the first match.
// Multiple matching instances therefore double only once per category.
inline std::optional<ChestDisarmEquipmentFlags> scanChestDisarmEquipment(
    GameTitle title, const std::array<ChestDisarmEquippedItem, 16> &equipped) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    for (const ChestDisarmEquippedItem &item : equipped)
        if (item.itemId < 0 || item.itemId > 580 || item.specialEnchantment < 0 ||
            (item.itemId == 0 && item.specialEnchantment != 0))
            return std::nullopt;

    ChestDisarmEquipmentFlags flags;
    flags.unbrokenItem410 = equipped[6].itemId == 410 && !equipped[6].broken;
    flags.unbrokenItem415 = (equipped[0].itemId == 415 && !equipped[0].broken) ||
                           (equipped[1].itemId == 415 && !equipped[1].broken);
    for (const ChestDisarmEquippedItem &item : equipped)
        if (item.specialEnchantment == 35 && !item.broken) {
            flags.unbrokenThieveryEnchantment = true;
            break;
        }
    return flags;
}

inline std::optional<int> stageChestDisarmScore(const ChestDisarmScoreInput &input) {
    if (input.title != GameTitle::MM6)
        return std::nullopt;
    int score = input.encodedSkill & 63;
    if (input.unbrokenItem410)
        score *= 2;
    if (input.unbrokenItem415)
        score *= 2;
    if (input.unbrokenThieveryEnchantment)
        score *= 2;
    if (input.hasTinker)
        score += 4;
    if (input.hasLocksmith)
        score += 6;
    if (input.hasBurglar)
        score += 8;
    const int masteryMultiplier = (input.encodedSkill & 128) ? 4 :
                                  (input.encodedSkill & 64) ? 3 : 2;
    return score * masteryMultiplier;
}

}  // namespace mm6
