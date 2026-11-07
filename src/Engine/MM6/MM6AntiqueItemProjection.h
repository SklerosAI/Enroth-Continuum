#pragma once

#include <bit>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6ItemRecord28.h"

namespace mm6 {

// MM6 special ID 58 is a value-only enchantment in the retained MM6 table.
// The original item picker writes its 1-based special ID at byte 12. Keep the
// exact record as the authority; no other special effect is inferred here.
struct AntiqueItemProjection {
    Item item;
    ItemRecord28 raw;
    ItemType sourceType = ITEM_TYPE_INVALID;
};

inline std::optional<AntiqueItemProjection> stageAntiqueItem(
    const ItemRecord28 &raw, const ItemTable &table, GameTitle title) {
    const std::uint32_t id = raw.itemNumberRaw();
    if (title != GameTitle::MM6 || id < 1 || id > 580 || raw.bonusRaw() ||
        raw.bonusStrengthRaw() ||
        raw.secondBonusRaw() != std::to_underlying(ITEM_ENCHANTMENT_ANTIQUE))
        return std::nullopt;

    const ItemData &data = table.items[static_cast<ItemId>(id)];
    if (data.iconName.empty() || data.rarity != RARITY_COMMON ||
        data.type < ITEM_TYPE_FIRST_SPECIAL_ENCHANTABLE ||
        data.type > ITEM_TYPE_LAST_SPECIAL_ENCHANTABLE ||
        data.baseValue < 0 || data.baseValue > std::numeric_limits<int>::max() / 10)
        return std::nullopt;

    const SpecialEnchantmentData &special = table.specialEnchantments[ITEM_ENCHANTMENT_ANTIQUE];
    if (special.valueMul != 10 || special.valueAdd != 0 ||
        special.itemSuffixOrPrefix.empty() || special.chanceByItemType[data.type] <= 0)
        return std::nullopt;

    AntiqueItemProjection result;
    result.raw = raw;
    result.sourceType = data.type;
    result.item.itemId = static_cast<ItemId>(id);
    result.item.mm6RawRecord = raw;
    result.item.specialEnchantment = ITEM_ENCHANTMENT_ANTIQUE;
    result.item.numCharges = std::bit_cast<std::int32_t>(raw.chargesRaw());
    result.item.flags = ItemFlags(raw.conditionRaw());
    result.item.maxCharges = raw.maxChargesRaw();
    return result;
}

inline std::optional<ItemRecord28> captureAntiqueItem(
    const AntiqueItemProjection &projection, const ItemTable &table, GameTitle title) {
    const auto source = stageAntiqueItem(projection.raw, table, title);
    const Item &item = projection.item;
    if (!source || source->sourceType != projection.sourceType ||
        item.mm6RawRecord != projection.raw || item.itemId != source->item.itemId ||
        item.specialEnchantment != ITEM_ENCHANTMENT_ANTIQUE ||
        item.standardEnchantment || item.standardEnchantmentStrength ||
        item.potionPower || item.goldAmount ||
        item.numCharges != source->item.numCharges ||
        item.maxCharges != source->item.maxCharges ||
        item.lichJarCharacterIndex != -1 || item.enchantmentExpirationTime.ticks() ||
        ((static_cast<std::uint32_t>(item.flags) ^ projection.raw.conditionRaw()) & ~3u))
        return std::nullopt;

    ItemRecord28 result = projection.raw;
    result.setWord(20, static_cast<std::uint32_t>(item.flags));
    return result;
}

} // namespace mm6
