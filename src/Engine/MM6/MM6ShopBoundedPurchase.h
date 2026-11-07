#pragma once

#include <optional>
#include <type_traits>
#include <utility>

#include "Engine/Objects/Inventory.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6BoundedItemProjection.h"
#include "MM6ShopStockPurchase.h"

namespace mm6 {

// Detached transfer of one already-paid stock record to a single MM6 backpack.
// The physical stock record and inventory publish only after every check passes.
// House prices, refresh, buyer selection and durable save ownership are separate.
inline std::optional<int> tryPurchaseBoundedStockItem(
    ShopStockRecord &source, CharacterInventory &inventory,
    const ItemTable &table, GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<CharacterInventory>);
    static_assert(std::is_nothrow_copy_assignable_v<ShopStockRecord>);

    if (title != GameTitle::MM6 || pItemTable != &table || inventory.capacity() != 138 ||
        source.bodyLocationRaw())
        return std::nullopt;

    auto projected = stageBoundedItem(source, table, title);
    if (!projected)
        return std::nullopt;
    const Item &item = boundedItem(*projected);
    const ItemData &data = table.items[item.itemId];
    if (data.rarity != RARITY_COMMON || data.type == ITEM_TYPE_GOLD ||
        data.type == ITEM_TYPE_POTION || item.mm6RawRecord != source)
        return std::nullopt;

    const Sizei size = table.itemSizes[item.itemId];
    if (size.w <= 0 || size.h <= 0 || size.w > 14 || size.h > 9)
        return std::nullopt;

    CharacterInventory stagedInventory = inventory;
    InventoryEntry added = stagedInventory.tryAdd(item);
    if (!added)
        return std::nullopt;
    const int addedIndex = added.index();

    ShopStockRecord stagedSource = source;
    const ShopStockRecord purchased = takePurchasedStockRecord(stagedSource, title);
    if (purchased != source || added->mm6RawRecord != purchased)
        return std::nullopt;

    inventory = std::move(stagedInventory);
    source = stagedSource;
    return addedIndex;
}

} // namespace mm6
