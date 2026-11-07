#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include "MM6BoundedInventoryProjection.h"
#include "MM6ShopBoundedPurchase.h"
#include "MM6ShopStockFields.h"

namespace mm6 {

// One already-paid, explicit physical stock slot to one explicit character.
// Every owner is copied and checked before the nonthrowing publication step.
inline std::optional<int> tryPurchasePhysicalStockItem(
    ShopStockFields &stock, std::size_t stockIndex,
    RawInventoryFields &rawInventory, std::size_t characterIndex,
    CharacterInventory &inventory, const ItemTable &table, GameTitle title) {
    static_assert(std::is_nothrow_move_assignable_v<ShopStockFields>);
    static_assert(std::is_nothrow_move_assignable_v<RawInventoryFields>);
    static_assert(std::is_nothrow_move_assignable_v<CharacterInventory>);

    if (title != GameTitle::MM6 || !stock.ready || !rawInventory.ready ||
        stockIndex >= stock.records.size() || characterIndex >= rawInventory.characters.size())
        return std::nullopt;

    // The live view must agree with the complete physical source, including
    // every unoccupied record and both index tables, before a sale can start.
    auto baseline = stageBoundedInventory(rawInventory.characters[characterIndex], table, title);
    if (!baseline)
        return std::nullopt;
    baseline->inventory() = inventory;
    auto captured = captureBoundedInventory(*baseline, table, title);
    if (!captured || *captured != rawInventory.characters[characterIndex])
        return std::nullopt;

    ShopStockFields stagedStock = stock;
    RawInventoryFields stagedRaw = rawInventory;
    CharacterInventory stagedInventory = inventory;
    const ShopStockRecord purchased = stagedStock.records[stockIndex];
    auto addedIndex = tryPurchaseBoundedStockItem(
        stagedStock.records[stockIndex], stagedInventory, table, title);
    if (!addedIndex || *addedIndex < 0 || *addedIndex >= int(RAW_INVENTORY_ITEM_SLOTS))
        return std::nullopt;

    InventoryConstEntry entry = stagedInventory.entry(*addedIndex);
    if (!entry || entry.zone() != INVENTORY_ZONE_GRID || !entry->mm6RawRecord ||
        *entry->mm6RawRecord != purchased)
        return std::nullopt;
    RawCharacterInventory &buyer = stagedRaw.characters[characterIndex];
    if (buyer.items[*addedIndex].itemNumberRaw() != 0)
        return std::nullopt;

    const Recti rect = entry.geometry();
    if (rect.w <= 0 || rect.h <= 0 || rect.x < 0 || rect.y < 0 ||
        rect.x + rect.w > 14 || rect.y + rect.h > 9)
        return std::nullopt;
    const int anchor = rect.y * 14 + rect.x;
    for (int y = rect.y; y < rect.y + rect.h; ++y)
        for (int x = rect.x; x < rect.x + rect.w; ++x)
            if (buyer.gridWords[y * 14 + x])
                return std::nullopt;

    buyer.items[*addedIndex] = purchased; // Full original shop-to-buyer copy.
    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        for (int x = rect.x; x < rect.x + rect.w; ++x) {
            const int cell = y * 14 + x;
            buyer.gridWords[cell] = cell == anchor ? *addedIndex + 1 :
                std::bit_cast<std::uint32_t>(-anchor - 1);
        }
    }

    auto replay = stageBoundedInventory(buyer, table, title);
    if (!replay)
        return std::nullopt;
    replay->inventory() = stagedInventory;
    auto recaptured = captureBoundedInventory(*replay, table, title);
    if (!recaptured || *recaptured != buyer)
        return std::nullopt;

    stock = std::move(stagedStock);
    rawInventory = std::move(stagedRaw);
    inventory = std::move(stagedInventory);
    return addedIndex;
}

} // namespace mm6
