#pragma once

#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

enum class ConsumableKind {
    Other,
    Herb,
    EmptyBottle,
    FilledBottle,
    UnsupportedBottle,
    InvalidTemplate,
};

// These boundaries are from the retained MM6 items.txt rows. The repeated
// bottle templates after the useitems.txt axis have no established use rule.
inline ConsumableKind classifyConsumable(ItemId id, ItemType type) {
    int value = static_cast<int>(id);
    if (value >= 160 && value <= 162)
        return type == ITEM_TYPE_REAGENT ? ConsumableKind::Herb : ConsumableKind::InvalidTemplate;
    if (value == 163)
        return type == ITEM_TYPE_POTION ? ConsumableKind::EmptyBottle : ConsumableKind::InvalidTemplate;
    if (value >= 164 && value <= 188)
        return type == ITEM_TYPE_POTION ? ConsumableKind::FilledBottle : ConsumableKind::InvalidTemplate;
    if (value >= 189 && value <= 196)
        return type == ITEM_TYPE_POTION ? ConsumableKind::UnsupportedBottle : ConsumableKind::InvalidTemplate;
    if (type == ITEM_TYPE_REAGENT || type == ITEM_TYPE_POTION)
        return ConsumableKind::InvalidTemplate;
    return ConsumableKind::Other;
}

inline bool shouldBlockDirectUse(ItemId id, ItemType type) {
    return classifyConsumable(id, type) != ConsumableKind::Other;
}

inline bool shouldBlockItemOnItemUse(ItemId id, ItemType type) {
    // MM6 spell scrolls occupy the numeric range used by MM7 reagents and
    // potions. The inherited item-on-item dispatch checks raw IDs.
    return shouldBlockDirectUse(id, type) ||
           (id >= ITEM_FIRST_REAGENT && id <= ITEM_LAST_POTION);
}

} // namespace mm6
