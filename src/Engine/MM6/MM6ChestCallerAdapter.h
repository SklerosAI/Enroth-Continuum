#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "Engine/MM6/MM6ChestItemFinalizer.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {

// The outer MM6 chest generator supplies one saved 0..99 probability to the
// first attempt and draws a new one for each additional attempt. This selector
// does not consume a draw or publish any chest state. Tier 7 bypasses it.
enum class ChestRewardSource { Empty, Gold, AnyItem, DirectArtifact };

inline std::optional<ChestRewardSource> chestRewardSource(GameTitle title,
                                                          ItemTreasureLevel tier,
                                                          int probability) {
    if (title != GameTitle::MM6 || probability < 0 || probability >= 100)
        return std::nullopt;
    const int level = std::to_underlying(tier);
    if (level == 7)
        return ChestRewardSource::DirectArtifact;
    if (level < 1 || level > 6)
        return std::nullopt;
    if (probability < 20)
        return ChestRewardSource::Empty;
    if (probability < 60)
        return ChestRewardSource::Gold;
    return ChestRewardSource::AnyItem;
}

// Gold comes from the original six MM6 tier bands, not from rnditems.txt.
struct ChestGoldPick {
    ItemId itemId = ITEM_NULL;
    int amount = 0;
    std::uint32_t nextRngState = 0;
};

inline std::optional<ChestGoldPick> pickChestGold(GameTitle title,
                                                  ItemTreasureLevel tier,
                                                  std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    static constexpr std::array<int, 6> base = {50, 100, 200, 500, 1000, 2000};
    static constexpr std::array<int, 6> span = {51, 101, 301, 501, 1001, 3001};
    static constexpr std::array<ItemId, 6> ids = {
        ITEM_GOLD_SMALL, ITEM_GOLD_SMALL, ITEM_GOLD_MEDIUM,
        ITEM_GOLD_MEDIUM, ITEM_GOLD_LARGE, ITEM_GOLD_LARGE};
    const int level = std::to_underlying(tier);
    if (level < 1 || level > 6)
        return std::nullopt;
    const int index = level - 1;
    return ChestGoldPick{ids[index], base[index] + static_cast<int>(nextChestRandom15(rngState) % span[index]),
                         rngState};
}

// A direct tier-seven artifact is a separate party-ownership picker. The
// original candidate loop's extra ID is unresolved; no selector is exposed.
inline bool needsDirectChestArtifactPicker(GameTitle title, ItemTreasureLevel tier) {
    return title == GameTitle::MM6 && tier == ITEM_TREASURE_LEVEL_7;
}

inline std::optional<ItemId> mm6ChestItemId(GameTitle title, int rawId) {
    if (title != GameTitle::MM6 || rawId < 1 || rawId > 580)
        return std::nullopt;
    return static_cast<ItemId>(rawId);
}

inline std::optional<ChestFinalizedItem> stageChestAnyFromItemTable(
    GameTitle title, ItemTreasureLevel tier, const ItemTable &table,
    const std::array<std::uint8_t, 30> &generatedArtifactFlags,
    std::uint32_t rngState) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    const int level = std::to_underlying(tier);
    if (level < 1 || level > 6)
        return std::nullopt;

    std::array<int, 399> weights{};
    for (int id = 1; id <= 399; ++id)
        weights[id - 1] = table.items[static_cast<ItemId>(id)].uChanceByTreasureLvl[tier];
    const auto base = pickChestBaseItemId(title, tier, weights, generatedArtifactFlags, rngState);
    if (!base)
        return std::nullopt;
    if (base->itemId >= 400)
        return finalizeChestAnyItem(title, tier, *base, {}, {});

    const auto mappedId = mm6ChestItemId(title, base->itemId);
    if (!mappedId)
        return std::nullopt;
    const ItemData &data = table.items[*mappedId];
    const int type = std::to_underlying(data.type);
    if (type < 0 || type > std::to_underlying(ITEM_TYPE_MISC) ||
        data.identifyAndRepairDifficulty < 0 || data.damageMod < 0 ||
        data.damageMod > std::numeric_limits<std::uint8_t>::max())
        return std::nullopt;
    ChestItemTemplateFields itemTemplate{
        base->itemId, data.type, data.identifyAndRepairDifficulty != 0,
        static_cast<std::uint8_t>(data.damageMod)};

    ChestItemFinalizationRules rules{};
    for (int i = 0; i < 6; ++i) {
        const auto rowTier = static_cast<ItemTreasureLevel>(i + 1);
        if (table.specialEnchantmentChanceForWeapons[rowTier] > 100 ||
            table.standardEnchantmentChanceForEquipment[rowTier] > 100 ||
            table.specialEnchantmentChanceForEquipment[rowTier] > 100)
            return std::nullopt;
        rules.weaponSpecialChance[i] = table.specialEnchantmentChanceForWeapons[rowTier];
        rules.equipmentStandardChance[i] = table.standardEnchantmentChanceForEquipment[rowTier];
        // MM6's retained second equipment row is a separate chance, not the
        // cumulative MM7 field described by the shared ItemTable comment.
        rules.equipmentSpecialChance[i] = table.specialEnchantmentChanceForEquipment[rowTier];
        const auto range = table.standardEnchantmentRangeByTreasureLevel[rowTier];
        rules.standardStrengthRange[i] = {range.front(), range.back()};
    }
    static constexpr std::array<Attribute, 14> attributes = {
        ATTRIBUTE_MIGHT, ATTRIBUTE_INTELLIGENCE, ATTRIBUTE_PERSONALITY, ATTRIBUTE_ENDURANCE,
        ATTRIBUTE_ACCURACY, ATTRIBUTE_SPEED, ATTRIBUTE_LUCK, ATTRIBUTE_HEALTH, ATTRIBUTE_MANA,
        ATTRIBUTE_AC_BONUS, ATTRIBUTE_RESIST_FIRE, ATTRIBUTE_RESIST_AIR, ATTRIBUTE_RESIST_WATER,
        ATTRIBUTE_RESIST_BODY};
    if (type >= std::to_underlying(ITEM_TYPE_ARMOUR) &&
        type <= std::to_underlying(ITEM_TYPE_AMULET)) {
        for (int i = 0; i < 14; ++i)
            rules.standardAttributeWeightsForType[i] =
                table.standardEnchantments[attributes[i]].chanceByItemType[data.type];
    }
    if (type <= std::to_underlying(ITEM_TYPE_AMULET)) {
        for (int i = 0; i < 59; ++i) {
            const auto &row = table.specialEnchantments[static_cast<ItemEnchantment>(i + 1)];
            rules.specialChancesForType[i] = {
                row.enchantmentLevel,
                static_cast<int>(row.chanceByItemType[data.type])};
        }
    }
    return finalizeChestAnyItem(title, tier, *base, itemTemplate, rules);
}

}  // namespace mm6
