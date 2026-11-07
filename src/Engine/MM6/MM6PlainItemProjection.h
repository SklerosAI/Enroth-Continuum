#pragma once

#include <bit>
#include <cstdint>
#include <optional>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6ItemRecord28.h"

namespace mm6 {

// A deliberately narrow view of one native MM6 record. The original raw
// record remains authoritative; this view accepts plain items and gold only.
// Enchantment and potion-power unions require separate source-backed rules.
struct PlainItemProjection {
    Item item;
    ItemRecord28 raw;
    ItemType sourceType = ITEM_TYPE_INVALID;
};

inline std::optional<PlainItemProjection> stagePlainItem(
    const ItemRecord28 &raw, const ItemTable &table, GameTitle title) {
    const std::uint32_t id = raw.itemNumberRaw();
    if (title != GameTitle::MM6 || id < 1 || id > 580 || raw.bonusRaw() ||
        raw.bonusStrengthRaw())
        return std::nullopt;

    const ItemData &data = table.items[static_cast<ItemId>(id)];
    if (data.iconName.empty() || data.type == ITEM_TYPE_INVALID ||
        (data.type != ITEM_TYPE_GOLD && raw.secondBonusRaw()))
        return std::nullopt;

    PlainItemProjection result;
    result.raw = raw;
    result.sourceType = data.type;
    result.item.itemId = static_cast<ItemId>(id);
    result.item.numCharges = std::bit_cast<std::int32_t>(raw.chargesRaw());
    result.item.flags = ItemFlags(raw.conditionRaw());
    result.item.maxCharges = raw.maxChargesRaw();
    if (data.type == ITEM_TYPE_GOLD)
        result.item.goldAmount = std::bit_cast<std::int32_t>(raw.secondBonusRaw());
    return result;
}

// Write only fields represented by the narrow view. In particular the body
// location, owner and skipped tail bytes remain exactly as the source held
// them. A changed item ID or unsupported logical field requires a new owner.
inline std::optional<ItemRecord28> capturePlainItem(
    const PlainItemProjection &projection, const ItemTable &table, GameTitle title) {
    const Item &item = projection.item;
    const std::uint32_t id = projection.raw.itemNumberRaw();
    const auto source = stagePlainItem(projection.raw, table, title);
    if (!source || source->sourceType != projection.sourceType ||
        item.itemId != static_cast<ItemId>(id) ||
        item.potionPower || item.standardEnchantment || item.standardEnchantmentStrength ||
        item.specialEnchantment != ITEM_ENCHANTMENT_NULL ||
        item.lichJarCharacterIndex != -1 || item.enchantmentExpirationTime.ticks() ||
        item.maxCharges < 0 || item.maxCharges > 255 ||
        ((static_cast<std::uint32_t>(item.flags) ^ projection.raw.conditionRaw()) & ~3u) ||
        (projection.sourceType != ITEM_TYPE_WAND &&
         (item.numCharges != source->item.numCharges || item.maxCharges != source->item.maxCharges)) ||
        (projection.sourceType != ITEM_TYPE_GOLD && item.goldAmount))
        return std::nullopt;

    ItemRecord28 result = projection.raw;
    result.setWord(16, std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(item.numCharges)));
    result.setWord(20, static_cast<std::uint32_t>(item.flags));
    result[25] = static_cast<std::uint8_t>(item.maxCharges);
    if (projection.sourceType == ITEM_TYPE_GOLD)
        result.setWord(12, std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(item.goldAmount)));
    return result;
}

} // namespace mm6
