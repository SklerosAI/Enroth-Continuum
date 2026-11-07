#pragma once

#include <bit>
#include <cstdint>
#include <limits>
#include <optional>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6ItemRecord28.h"
#include "MM6StandardBonusMap.h"

namespace mm6 {

// Original MM6 standard-enchanted equipment, with the exact raw record kept
// beside the runtime view. This does not cover special bonuses or potions.
struct StandardItemProjection {
    Item item;
    ItemRecord28 raw;
    ItemType sourceType = ITEM_TYPE_INVALID;
};

// The shared Item::GetValue() evaluates baseValue + 100 * strength in int.
// Keep both imported and edited bonuses inside that representable range.
inline bool standardStrengthFitsValue(std::int32_t strength, int baseValue) {
    if (strength <= 0 || strength > std::numeric_limits<int>::max() / 100)
        return false;
    const std::int64_t value = static_cast<std::int64_t>(baseValue) +
                               100 * static_cast<std::int64_t>(strength);
    return value >= std::numeric_limits<int>::min() &&
           value <= std::numeric_limits<int>::max();
}

inline std::optional<StandardItemProjection> stageStandardItem(
    const ItemRecord28 &raw, const ItemTable &table, GameTitle title) {
    const std::uint32_t id = raw.itemNumberRaw();
    const auto attribute = attributeForStandardBonus(raw.bonusRaw());
    const std::int32_t strength = std::bit_cast<std::int32_t>(raw.bonusStrengthRaw());
    if (title != GameTitle::MM6 || id < 1 || id > 580 || !attribute ||
        raw.secondBonusRaw())
        return std::nullopt;

    const ItemData &data = table.items[static_cast<ItemId>(id)];
    if (data.iconName.empty() || data.rarity != RARITY_COMMON ||
        data.type < ITEM_TYPE_FIRST_NORMAL_ENCHANTABLE ||
        data.type > ITEM_TYPE_LAST_NORMAL_ENCHANTABLE ||
        !standardStrengthFitsValue(strength, data.baseValue))
        return std::nullopt;

    StandardItemProjection result;
    result.raw = raw;
    result.sourceType = data.type;
    result.item.itemId = static_cast<ItemId>(id);
    result.item.standardEnchantment = *attribute;
    result.item.standardEnchantmentStrength = strength;
    result.item.numCharges = std::bit_cast<std::int32_t>(raw.chargesRaw());
    result.item.flags = ItemFlags(raw.conditionRaw());
    result.item.maxCharges = raw.maxChargesRaw();
    return result;
}

inline std::optional<ItemRecord28> captureStandardItem(
    const StandardItemProjection &projection, const ItemTable &table, GameTitle title) {
    const auto source = stageStandardItem(projection.raw, table, title);
    const Item &item = projection.item;
    const auto bonus = item.standardEnchantment
        ? standardBonusForAttribute(*item.standardEnchantment) : std::nullopt;
    if (!source || source->sourceType != projection.sourceType || !bonus ||
        item.itemId != source->item.itemId ||
        !standardStrengthFitsValue(item.standardEnchantmentStrength,
                                   table.items[item.itemId].baseValue) ||
        item.potionPower || item.goldAmount || item.specialEnchantment != ITEM_ENCHANTMENT_NULL ||
        item.numCharges != source->item.numCharges || item.maxCharges != source->item.maxCharges ||
        item.lichJarCharacterIndex != -1 || item.enchantmentExpirationTime.ticks() ||
        ((static_cast<std::uint32_t>(item.flags) ^ projection.raw.conditionRaw()) & ~3u))
        return std::nullopt;

    ItemRecord28 result = projection.raw;
    result.setWord(4, *bonus);
    result.setWord(8, std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(item.standardEnchantmentStrength)));
    result.setWord(20, static_cast<std::uint32_t>(item.flags));
    return result;
}

} // namespace mm6
