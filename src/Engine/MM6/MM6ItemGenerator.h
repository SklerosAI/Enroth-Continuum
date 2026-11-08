#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Engine/MM6/MM6ChestBaseItemPicker.h"
#include "Engine/MM6/MM6ChestItemFinalizer.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6StandardBonusMap.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {

namespace detail {

// The original items.txt loader stores weapon1or2 as 0 and every unlisted name as 19. The shared loader skips the
// rows with an empty icon, 299 and 399, whose empty type and skill cells the original reads as 19 and 13.
inline int originalItemTypeCode(ItemType type) {
    const int value = std::to_underlying(type);
    return value >= std::to_underlying(ITEM_TYPE_SINGLE_HANDED) && value <= std::to_underlying(ITEM_TYPE_GOLD) ? value : 19;
}

inline int originalItemSkillCode(Skill skill) {
    switch (skill) {
    case SKILL_CLUB: return 0;
    case SKILL_STAFF: return 1;
    case SKILL_SWORD: return 2;
    case SKILL_DAGGER: return 3;
    case SKILL_AXE: return 4;
    case SKILL_SPEAR: return 5;
    case SKILL_BOW: return 6;
    case SKILL_MACE: return 7;
    case SKILL_BLASTER: return 8;
    case SKILL_SHIELD: return 9;
    case SKILL_LEATHER: return 10;
    case SKILL_CHAIN: return 11;
    case SKILL_PLATE: return 12;
    default: return 13;
    }
}

struct ItemKindFilter {
    bool bySkill = false;
    int code = 0;
};

// Requested kinds 20..43 go through the original jump table, any other nonzero kind is an item type plus one.
inline ItemKindFilter itemKindFilter(int requestedKind) {
    static constexpr std::array<ItemKindFilter, 24> table = {{
        {false, 0}, {false, 3}, {true, 13}, {true, 2}, {true, 3}, {true, 4}, {true, 5}, {true, 6},
        {true, 7}, {true, 0}, {true, 1}, {true, 10}, {true, 11}, {true, 12}, {false, 4}, {false, 5},
        {false, 6}, {false, 7}, {false, 8}, {false, 9}, {false, 10}, {false, 11}, {false, 12}, {false, 15}
    }};
    if (requestedKind >= 20 && requestedKind <= 43)
        return table[requestedKind - 20];
    return {false, requestedKind - 1};
}

/**
 * Filtered base-ID stage of the original MM6 random item generator. Every regular item of the requested kind is a
 * candidate, zero weights included, and a draw happens only when the candidates' weights sum to more than zero.
 *
 * @param table                         MM6 item table.
 * @param treasureLevel                 Treasure level 1..6.
 * @param requestedKind                 Nonzero requested kind.
 * @param rngState                      MM6 gameplay LCG state.
 * @return                              Picked ID with the next LCG state, or `std::nullopt` without candidates.
 */
inline std::optional<ChestFinalizedItem> pickFilteredItemId(const ItemTable &table, ItemTreasureLevel treasureLevel,
                                                            int requestedKind, std::uint32_t rngState) {
    const ItemKindFilter filter = itemKindFilter(requestedKind);
    std::vector<int> candidates;
    std::int64_t total = 0;
    for (int id = 1; id <= 399; ++id) {
        const ItemData &data = table.items[static_cast<ItemId>(id)];
        const int code = filter.bySkill ? originalItemSkillCode(data.skill) : originalItemTypeCode(data.type);
        if (code != filter.code)
            continue;
        const int weight = data.uChanceByTreasureLvl[treasureLevel];
        if (weight < 0 || weight > 255)
            return std::nullopt;
        candidates.push_back(id);
        total += weight;
    }
    if (candidates.empty()) // The original would go on with item 0.
        return std::nullopt;

    ChestFinalizedItem result;
    result.nextRngState = rngState;
    int roll = 0;
    if (total) {
        roll = nextChestRandom15(result.nextRngState) % static_cast<int>(total);
        result.drawsConsumed = 1;
    }
    std::int64_t cumulative = 0;
    for (int id : candidates) {
        result.itemId = id;
        cumulative += table.items[static_cast<ItemId>(id)].uChanceByTreasureLvl[treasureLevel];
        if (cumulative >= roll)
            break;
    }
    return result;
}

} // namespace detail

/**
 * Template fields and bonus tables that the original generator's post-ID stage reads for one regular item.
 *
 * @param table                         MM6 item table.
 * @param itemId                        Regular item ID, 1..399.
 * @return                              Inputs for `finalizeRegularItem`, or `std::nullopt` for out-of-range rows.
 */
inline std::optional<std::pair<ChestItemTemplateFields, ChestItemFinalizationRules>> itemFinalizationInputs(
    const ItemTable &table, int itemId) {
    if (itemId < 1 || itemId > 399)
        return std::nullopt;
    const ItemData &data = table.items[static_cast<ItemId>(itemId)];
    const int type = std::to_underlying(data.type);
    if (type < 0 || type > std::to_underlying(ITEM_TYPE_MISC) ||
        data.identifyAndRepairDifficulty < 0 || data.identifyAndRepairDifficulty > 255 ||
        data.damageMod < 0 || data.damageMod > 255)
        return std::nullopt;
    ChestItemTemplateFields itemTemplate{itemId, data.type, data.identifyAndRepairDifficulty != 0,
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
        // MM6's second equipment row is a separate chance, not the cumulative MM7 field described by the shared
        // ItemTable comment.
        rules.equipmentSpecialChance[i] = table.specialEnchantmentChanceForEquipment[rowTier];
        const auto range = table.standardEnchantmentRangeByTreasureLevel[rowTier];
        rules.standardStrengthRange[i] = {range.front(), range.back()};
    }
    if (type >= std::to_underlying(ITEM_TYPE_ARMOUR) && type <= std::to_underlying(ITEM_TYPE_AMULET)) {
        for (int i = 0; i < 14; ++i)
            rules.standardAttributeWeightsForType[i] =
                table.standardEnchantments[kStandardBonusAttributes[i]].chanceByItemType[data.type];
    }
    if (type <= std::to_underlying(ITEM_TYPE_AMULET)) {
        for (int i = 0; i < static_cast<int>(rules.specialChancesForType.size()); ++i) {
            const auto &row = table.specialEnchantments[static_cast<ItemEnchantment>(i + 1)];
            rules.specialChancesForType[i] = {row.enchantmentLevel, static_cast<int>(row.chanceByItemType[data.type])};
        }
    }
    return std::pair{itemTemplate, rules};
}

/**
 * The original MM6 random item generator. Draws from the MM6 gameplay LCG exactly as the original does, and returns
 * the staged item fields with the next LCG state and artifact flags. Nothing is published.
 *
 * @param title                         Must be `GameTitle::MM6`.
 * @param table                         MM6 item table.
 * @param treasureLevel                 Treasure level 1..6.
 * @param requestedKind                 Any item, or a type or skill group. Only `RANDOM_ITEM_ANY` can roll a
 *                                      generated artifact.
 * @param generatedArtifactFlags        Generated flags of artifacts 400..429.
 * @param rngState                      MM6 gameplay LCG state.
 * @return                              Generated item, or `std::nullopt` where the original would read outside its
 *                                      tables or divide by zero.
 */
inline std::optional<ChestFinalizedItem> generateItem(GameTitle title, const ItemTable &table,
                                                      ItemTreasureLevel treasureLevel, RandomItemType requestedKind,
                                                      const std::array<std::uint8_t, 30> &generatedArtifactFlags,
                                                      std::uint32_t rngState) {
    const int tier = std::to_underlying(treasureLevel);
    const int kind = std::to_underlying(requestedKind);
    if (title != GameTitle::MM6 || tier < 1 || tier > 6 || kind < 0 || kind > 255)
        return std::nullopt;

    std::optional<ChestFinalizedItem> picked;
    if (requestedKind == RANDOM_ITEM_ANY) {
        std::array<int, 399> weights{};
        for (int id = 1; id <= 399; ++id)
            weights[id - 1] = table.items[static_cast<ItemId>(id)].uChanceByTreasureLvl[treasureLevel];
        const auto base = pickChestBaseItemId(title, treasureLevel, weights, generatedArtifactFlags, rngState);
        if (!base)
            return std::nullopt;
        if (base->itemId >= 400)
            return finalizeChestAnyItem(title, treasureLevel, *base, {}, {});
        picked = ChestFinalizedItem{};
        picked->itemId = base->itemId;
        picked->nextRngState = base->nextRngState;
        picked->drawsConsumed = base->drawsConsumed;
        picked->generatedArtifactFlags = base->generatedArtifactFlags;
    } else {
        for (std::uint8_t flag : generatedArtifactFlags)
            if (flag > 1)
                return std::nullopt;
        picked = detail::pickFilteredItemId(table, treasureLevel, kind, rngState);
        if (!picked)
            return std::nullopt;
        picked->generatedArtifactFlags = generatedArtifactFlags;
    }

    const auto inputs = itemFinalizationInputs(table, picked->itemId);
    if (!inputs)
        return std::nullopt;
    return finalizeRegularItem(treasureLevel, *picked, inputs->first, inputs->second);
}

/**
 * Runtime view of a generated MM6 item.
 *
 * @param fields                        Item fields from `generateItem`.
 * @return                              Item with its one-based MM6 bonus indices mapped to runtime enums.
 */
Item generatedRuntimeItem(const ChestFinalizedItem &fields);

/**
 * Generated item staged against the live MM6 title state. The caller publishes `nextState` only once the item has
 * been placed.
 */
struct StagedTitleItem {
    Item item;
    ChestTitleState nextState;
};

/**
 * Runs `generateItem` on the MM6 gameplay LCG and artifact flags held in `state`.
 *
 * @param state                         MM6 title state, must be ready.
 * @param table                         MM6 item table.
 * @param treasureLevel                 Treasure level 1..6.
 * @param requestedKind                 Requested item kind.
 * @return                              Staged item and title state. Throws if the state isn't ready or the original
 *                                      generator would fail.
 */
StagedTitleItem stageTitleItem(const ChestTitleState &state, const ItemTable &table, ItemTreasureLevel treasureLevel,
                               RandomItemType requestedKind);

} // namespace mm6
