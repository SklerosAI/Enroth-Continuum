#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "Engine/MM6/MM6ChestBaseItemPicker.h"

namespace mm6 {

// Caller-provided rows for the selected regular item's type. The live MM6
// chest path does not construct these yet; they are a detached PE contract.
struct ChestSpecialChance {
    int levelCode = 0;
    int weightForItemType = 0;
};

struct ChestItemFinalizationRules {
    std::array<int, 6> weaponSpecialChance = {};
    std::array<int, 6> equipmentStandardChance = {};
    std::array<int, 6> equipmentSpecialChance = {};
    std::array<std::pair<int, int>, 6> standardStrengthRange = {};
    std::array<int, 14> standardAttributeWeightsForType = {};
    std::array<ChestSpecialChance, 59> specialChancesForType = {};
};

struct ChestItemTemplateFields {
    int itemId = 0;
    ItemType type = ITEM_TYPE_INVALID;
    bool requiresIdentification = false;
    std::uint8_t wandChargeBase = 0;
};

// Only the item fields written by original MM6 ANY random generation. This
// picker does not assign potion power or maximum charges; later sources and
// chest placement require separate contracts.
struct ChestFinalizedItem {
    int itemId = 0;
    int standardAttributeIndex = 0;  // Original 1-based field at item+4.
    int standardStrength = 0;        // item+8.
    int specialEnchantmentId = 0;    // Original 1-based field at item+12.
    int numCharges = 0;              // item+16.
    std::uint32_t flags = 0;         // item+20.
    std::uint32_t nextRngState = 0;
    int drawsConsumed = 0;           // Includes the preceding base-ID draws.
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
};

inline std::optional<ChestFinalizedItem> finalizeChestAnyItem(
    GameTitle title, ItemTreasureLevel treasureLevel,
    const ChestBaseItemPick &base, const ChestItemTemplateFields &itemTemplate,
    const ChestItemFinalizationRules &rules) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    const int tier = std::to_underlying(treasureLevel);
    if (tier < 1 || tier > 6 || base.drawsConsumed < 2)
        return std::nullopt;

    int generatedCount = 0;
    for (std::uint8_t flag : base.generatedArtifactFlags) {
        if (flag > 1)
            return std::nullopt;
        generatedCount += flag;
    }

    ChestFinalizedItem result;
    result.itemId = base.itemId;
    result.nextRngState = base.nextRngState;
    result.drawsConsumed = base.drawsConsumed;
    result.generatedArtifactFlags = base.generatedArtifactFlags;

    if (base.itemId >= 400 && base.itemId <= 429) {
        if (tier != 6 || base.drawsConsumed != 2 || generatedCount > 13 ||
            !base.generatedArtifactFlags[base.itemId - 400])
            return std::nullopt;
        return result;  // The original artifact success exits before regular field writes.
    }
    if (base.itemId < 1 || base.itemId > 399 || itemTemplate.itemId != base.itemId ||
        base.drawsConsumed != (tier == 6 ? 3 : 2))
        return std::nullopt;

    const int type = std::to_underlying(itemTemplate.type);
    if (type < 0 || type > std::to_underlying(ITEM_TYPE_MISC))
        return std::nullopt;
    for (int i = 0; i < 6; ++i) {
        if (rules.weaponSpecialChance[i] < 0 || rules.weaponSpecialChance[i] > 100 ||
            rules.equipmentStandardChance[i] < 0 || rules.equipmentStandardChance[i] > 100 ||
            rules.equipmentSpecialChance[i] < 0 || rules.equipmentSpecialChance[i] > 100 ||
            rules.equipmentStandardChance[i] + rules.equipmentSpecialChance[i] > 100 ||
            rules.standardStrengthRange[i].first < 0 ||
            rules.standardStrengthRange[i].second < rules.standardStrengthRange[i].first)
            return std::nullopt;
    }
    std::int64_t attributeWeightSum = 0;
    for (int weight : rules.standardAttributeWeightsForType) {
        if (weight < 0)
            return std::nullopt;
        attributeWeightSum += weight;
    }
    if (attributeWeightSum > std::numeric_limits<std::int32_t>::max())
        return std::nullopt;
    for (const ChestSpecialChance &chance : rules.specialChancesForType) {
        if (chance.levelCode < 0 || chance.levelCode > 3 || chance.weightForItemType < 0)
            return std::nullopt;
    }

    // The original item table checks a nonzero identification difficulty.
    result.flags = itemTemplate.requiresIdentification ? 0u : 1u;
    auto draw = [&result]() {
        ++result.drawsConsumed;
        return nextChestRandom15(result.nextRngState);
    };
    auto pickSpecial = [&]() -> bool {
        std::int64_t total = 0;
        for (const ChestSpecialChance &chance : rules.specialChancesForType) {
            const int code = chance.levelCode;
            const bool eligible = (tier == 3 && (code == 0 || code == 1)) ||
                                  (tier == 4 && code <= 2) ||
                                  (tier == 5 && code >= 1) ||
                                  (tier == 6 && code == 3);
            if (eligible)
                total += chance.weightForItemType;
        }
        if (total <= 0 || total > std::numeric_limits<std::int32_t>::max())
            return false;
        const int roll = draw() % static_cast<int>(total);
        int cumulative = 0;
        for (int i = 0; i < 59; ++i) {
            const ChestSpecialChance &chance = rules.specialChancesForType[i];
            const int code = chance.levelCode;
            const bool eligible = (tier == 3 && (code == 0 || code == 1)) ||
                                  (tier == 4 && code <= 2) ||
                                  (tier == 5 && code >= 1) ||
                                  (tier == 6 && code == 3);
            if (eligible && chance.weightForItemType > 0) {
                cumulative += chance.weightForItemType;
                if (cumulative > roll) {
                    result.specialEnchantmentId = i + 1;
                    return true;
                }
            }
        }
        return false;
    };

    if (type <= std::to_underlying(ITEM_TYPE_BOW)) {
        const int chance = rules.weaponSpecialChance[tier - 1];
        if (chance && draw() % 100 < chance && !pickSpecial())
            return std::nullopt;
    } else if (type <= std::to_underlying(ITEM_TYPE_AMULET)) {
        const int standardChance = rules.equipmentStandardChance[tier - 1];
        if (standardChance) {
            const int roll = draw() % 100;
            if (roll < standardChance) {
                if (attributeWeightSum <= 0)
                    return std::nullopt;
                const int attributeRoll = draw() % static_cast<int>(attributeWeightSum);
                int cumulative = 0;
                for (int i = 0; i < 14; ++i) {
                    cumulative += rules.standardAttributeWeightsForType[i];
                    if (cumulative >= attributeRoll) {
                        result.standardAttributeIndex = i + 1;
                        break;
                    }
                }
                const auto [low, high] = rules.standardStrengthRange[tier - 1];
                const std::int64_t span = static_cast<std::int64_t>(high) - low + 1;
                result.standardStrength = low + draw() % span;
            } else if (roll < standardChance + rules.equipmentSpecialChance[tier - 1] &&
                       !pickSpecial()) {
                return std::nullopt;
            }
        }
    } else if (type == std::to_underlying(ITEM_TYPE_WAND)) {
        result.numCharges = itemTemplate.wandChargeBase + draw() % 6;
    }
    return result;
}

}  // namespace mm6
