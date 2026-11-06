#pragma once

#include <vector>

#include "Engine/Tables/ItemTable.h"

namespace mm6 {

// Empty icon names belong to unused placeholder templates. A named template
// must have an image; otherwise LoadItemSizes would silently assign 1x1.
template <typename Exists>
std::vector<ItemId> collectMissingItemIconIds(const ItemTable &table, Exists exists) {
    std::vector<ItemId> missing;
    for (ItemId itemId : table.items.indices()) {
        const std::string &iconName = table.items[itemId].iconName;
        if (!iconName.empty() && !exists(iconName))
            missing.push_back(itemId);
    }
    return missing;
}

} // namespace mm6
