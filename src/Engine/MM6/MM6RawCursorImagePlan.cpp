#include "MM6RawCursorImagePlan.h"

#include <stdexcept>

#include "MM6BoundedItemProjection.h"

namespace mm6 {
namespace {

bool hasItemPayload(const Item &item) {
    return item.itemId != ITEM_NULL || item.potionPower || item.goldAmount ||
           item.standardEnchantment || item.standardEnchantmentStrength ||
           item.specialEnchantment != ITEM_ENCHANTMENT_NULL || item.numCharges ||
           item.maxCharges || item.flags || item.lichJarCharacterIndex != -1 ||
           item.enchantmentExpirationTime.ticks() || item.mm6RawRecord;
}

} // namespace

RawCursorImagePlan stageRawCursorImagePlan(const Party &party,
                                           const RawHeldCursorOwner &cursor,
                                           const ItemTable &itemTable,
                                           GameTitle title) {
    if (!validRawHeldCursorOwner(cursor, title))
        throw std::runtime_error("MM6 cursor image plan needs a ready raw HELD owner");
    const Item &item = party.pPickedItem;
    const std::uint32_t rawId = cursor.record.itemNumberRaw();
    if (!rawId) {
        if (hasItemPayload(item))
            throw std::runtime_error("MM6 empty raw cursor disagrees with Party held item");
        return {false, ITEM_NULL, "MICON1", HeldCursorTint::Normal, Pointi{}};
    }
    if (item.itemId != static_cast<ItemId>(rawId) ||
        item.mm6RawRecord != cursor.record)
        throw std::runtime_error("MM6 raw cursor disagrees with Party held item");
    auto projected = stageBoundedItem(cursor.record, itemTable, title);
    if (!projected)
        throw std::runtime_error("MM6 cursor image plan cannot project physical item");
    setBoundedItem(*projected, item);
    const auto replay = captureBoundedItem(*projected, itemTable, title);
    if (!replay || *replay != cursor.record)
        throw std::runtime_error("MM6 cursor image plan cannot replay physical item");
    const std::string &iconName = itemTable.items[item.itemId].iconName;
    if (iconName.empty())
        throw std::runtime_error("MM6 cursor image plan lacks a template icon");
    const HeldCursorTint tint = item.IsBroken() ? HeldCursorTint::Broken :
        item.IsIdentified() ? HeldCursorTint::Normal : HeldCursorTint::Unidentified;
    return {true, item.itemId, iconName, tint, std::nullopt};
}

} // namespace mm6
