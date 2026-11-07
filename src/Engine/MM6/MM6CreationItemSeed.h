#pragma once

#include <bit>
#include <cstdint>
#include <optional>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Objects/Inventory.h"

#include "MM6BoundedPartyInventoryProjection.h"
#include "MM6RawHeldCursorOwner.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6StandardBonusMap.h"

namespace mm6 {

struct SeededCreationItems {
    Party party;
    RawInventoryFields inventory;
    RawHeldCursorOwner cursor;
};

inline bool sameCreationItem(const Item &left, const Item &right) {
    return left.itemId == right.itemId && left.potionPower == right.potionPower &&
           left.goldAmount == right.goldAmount &&
           left.standardEnchantment == right.standardEnchantment &&
           left.standardEnchantmentStrength == right.standardEnchantmentStrength &&
           left.specialEnchantment == right.specialEnchantment &&
           left.numCharges == right.numCharges && left.maxCharges == right.maxCharges &&
           left.flags == right.flags &&
           left.lichJarCharacterIndex == right.lichJarCharacterIndex &&
           left.enchantmentExpirationTime == right.enchantmentExpirationTime;
}

// Create canonical native records for a *fresh* MM6 creation Party. A source
// record already carried by an item belongs to its previous physical owner;
// this seed must never replace or silently reinterpret it. Unknown native
// unions and generator outputs outside the bounded item projection reject.
inline std::optional<SeededCreationItems> seedMM6CreationItems(
    const Party &source, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || pItemTable != &table ||
        (engine && engine->gameTitle() != title) || source.pPickedItem.itemId != ITEM_NULL ||
        !sameCreationItem(source.pPickedItem, Item{}) || source.pPickedItem.mm6RawRecord)
        return std::nullopt;

    SeededCreationItems result{source, {}, *freshRawHeldCursorOwner(title)};
    result.inventory.ready = true;
    for (std::size_t character = 0; character < result.party.pCharacters.size(); ++character) {
        const CharacterInventory &original = source.pCharacters[character].inventory;
        CharacterInventory expanded = CharacterInventory::forMM6();
        RawCharacterInventory &raw = result.inventory.characters[character];
        if (original.capacity() > static_cast<int>(RAW_INVENTORY_ITEM_SLOTS))
            return std::nullopt;

        for (InventoryConstEntry entry : original.entries()) {
            const Item &item = *entry;
            const int index = entry.index();
            if (index < 0 || index >= static_cast<int>(RAW_INVENTORY_ITEM_SLOTS) ||
                item.mm6RawRecord || item.itemId < ITEM_FIRST_VALID ||
                std::to_underlying(item.itemId) > 580 ||
                item.potionPower || item.goldAmount ||
                item.lichJarCharacterIndex != -1 ||
                item.enchantmentExpirationTime.ticks() ||
                item.maxCharges < 0 || item.maxCharges > 255)
                return std::nullopt;

            ItemRecord28 record;
            record.setWord(0, std::to_underlying(item.itemId));
            if (item.standardEnchantment) {
                auto bonus = standardBonusForAttribute(*item.standardEnchantment);
                if (!bonus || item.specialEnchantment != ITEM_ENCHANTMENT_NULL)
                    return std::nullopt;
                record.setWord(4, *bonus);
                record.setWord(8, std::bit_cast<std::uint32_t>(
                    static_cast<std::int32_t>(item.standardEnchantmentStrength)));
            } else if (item.specialEnchantment != ITEM_ENCHANTMENT_NULL) {
                if (item.specialEnchantment != ITEM_ENCHANTMENT_ANTIQUE ||
                    item.standardEnchantmentStrength)
                    return std::nullopt;
                record.setWord(12, std::to_underlying(item.specialEnchantment));
            } else if (item.standardEnchantmentStrength) {
                return std::nullopt;
            }
            record.setWord(16, std::bit_cast<std::uint32_t>(
                static_cast<std::int32_t>(item.numCharges)));
            record.setWord(20, static_cast<std::uint32_t>(item.flags));
            record[25] = static_cast<std::uint8_t>(item.maxCharges);
            auto projection = stageBoundedItem(record, table, title);
            if (!projection || !sameCreationItem(item, boundedItem(*projection)))
                return std::nullopt;

            bool restored = false;
            if (entry.zone() == INVENTORY_ZONE_GRID) {
                const Recti rect = entry.geometry();
                if (rect.w <= 0 || rect.h <= 0 || rect.x < 0 || rect.y < 0 ||
                    rect.x + rect.w > 14 || rect.y + rect.h > 9)
                    return std::nullopt;
                const int anchor = rect.y * 14 + rect.x;
                for (int y = rect.y; y < rect.y + rect.h; ++y)
                    for (int x = rect.x; x < rect.x + rect.w; ++x) {
                        const int cell = y * 14 + x;
                        if (raw.gridWords[cell])
                            return std::nullopt;
                        raw.gridWords[cell] = cell == anchor ? index + 1 :
                            std::bit_cast<std::uint32_t>(-anchor - 1);
                    }
                restored = static_cast<bool>(expanded.tryRestoreAt(
                    index, Pointi(rect.x, rect.y), boundedItem(*projection)));
            } else {
                // Fresh creation grants enter backpacks. Any equipped item
                // requires a separate body-location source contract.
                return std::nullopt;
            }
            if (!restored)
                return std::nullopt;
            raw.items[index] = record;
        }
        result.party.pCharacters[character].inventory = std::move(expanded);
    }

    auto staged = stageBoundedPartyInventory(result.inventory, table, title);
    if (!staged)
        return std::nullopt;
    try {
        StagedRawPartyInventories check{result.party, result.inventory};
        if (captureRawPartyInventories(check, table, title) != result.inventory)
            return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    return result;
}

} // namespace mm6
