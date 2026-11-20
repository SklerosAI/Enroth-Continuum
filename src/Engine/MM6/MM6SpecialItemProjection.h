#pragma once

#include <bit>
#include <cstdint>
#include <limits>
#include <optional>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6ItemRecord28.h"

namespace mm6 {

/**
 * Original MM6 item with a special bonus, with the exact raw record kept beside the runtime view. The record's word at
 * byte 12 is the one-based `spcitems.txt` row, which the runtime view keeps as `Item::specialEnchantment`.
 */
struct SpecialItemProjection {
    Item item;
    ItemRecord28 raw;
    ItemType sourceType = ITEM_TYPE_INVALID;
};

/**
 * @param raw                           Original 28-byte item record.
 * @param table                         The loaded MM6 item table.
 * @param title                         Must be `GameTitle::MM6`.
 * @return                              Runtime view of a common item with a special bonus the original generator can
 *                                      give its type, or `std::nullopt` for any other record.
 */
inline std::optional<SpecialItemProjection> stageSpecialItem(
    const ItemRecord28 &raw, const ItemTable &table, GameTitle title) {
    const std::uint32_t id = raw.itemNumberRaw();
    const std::uint32_t special = raw.secondBonusRaw();
    if (title != GameTitle::MM6 || id < 1 || id > 580 || raw.bonusRaw() || raw.bonusStrengthRaw() ||
        special < 1 || special > 58) // MM6.exe loads 59 rows of spcitems.txt, but its generator picks from the first 58.
        return std::nullopt;

    const ItemData &data = table.items[static_cast<ItemId>(id)];
    if (data.iconName.empty() || data.rarity != RARITY_COMMON ||
        data.type < ITEM_TYPE_FIRST_SPECIAL_ENCHANTABLE ||
        data.type > ITEM_TYPE_LAST_SPECIAL_ENCHANTABLE)
        return std::nullopt;

    const SpecialEnchantmentData &row = table.specialEnchantments[static_cast<ItemEnchantment>(special)];
    const std::int64_t value = static_cast<std::int64_t>(data.baseValue) * row.valueMul + row.valueAdd; // As Item::GetValue has it.
    if (row.itemSuffixOrPrefix.empty() || row.chanceByItemType[data.type] <= 0 ||
        value < 0 || value > std::numeric_limits<int>::max())
        return std::nullopt;

    SpecialItemProjection result;
    result.raw = raw;
    result.sourceType = data.type;
    result.item.itemId = static_cast<ItemId>(id);
    result.item.mm6RawRecord = raw;
    result.item.specialEnchantment = static_cast<ItemEnchantment>(special);
    result.item.numCharges = std::bit_cast<std::int32_t>(raw.chargesRaw());
    result.item.flags = ItemFlags(raw.conditionRaw());
    result.item.maxCharges = raw.maxChargesRaw();
    return result;
}

/**
 * @param projection                    Projection whose item may have been edited after staging.
 * @param table                         The loaded MM6 item table.
 * @param title                         Must be `GameTitle::MM6`.
 * @return                              The staged record with the item's identified and broken bits, or `std::nullopt`
 *                                      if any other field of the item changed.
 */
inline std::optional<ItemRecord28> captureSpecialItem(
    const SpecialItemProjection &projection, const ItemTable &table, GameTitle title) {
    const auto source = stageSpecialItem(projection.raw, table, title);
    const Item &item = projection.item;
    if (!source || source->sourceType != projection.sourceType ||
        item.mm6RawRecord != projection.raw || item.itemId != source->item.itemId ||
        item.specialEnchantment != source->item.specialEnchantment ||
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
