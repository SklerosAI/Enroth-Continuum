#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "MM6AntiqueItemProjection.h"
#include "MM6PlainItemProjection.h"
#include "MM6StandardItemProjection.h"

namespace mm6 {

using BoundedItemProjection = std::variant<PlainItemProjection, StandardItemProjection,
                                           AntiqueItemProjection>;

inline std::optional<BoundedItemProjection> stageBoundedItem(
    const ItemRecord28 &raw, const ItemTable &table, GameTitle title) {
    if (raw.bonusRaw() || raw.bonusStrengthRaw()) {
        auto standard = stageStandardItem(raw, table, title);
        if (!standard)
            return std::nullopt;
        return BoundedItemProjection(std::move(*standard));
    }
    if (raw.secondBonusRaw()) {
        auto antique = stageAntiqueItem(raw, table, title);
        if (antique)
            return BoundedItemProjection(std::move(*antique));
    }
    auto plain = stagePlainItem(raw, table, title);
    if (!plain)
        return std::nullopt;
    return BoundedItemProjection(std::move(*plain));
}

inline const Item &boundedItem(const BoundedItemProjection &projection) {
    return std::visit([](const auto &value) -> const Item & { return value.item; }, projection);
}

inline void setBoundedItem(BoundedItemProjection &projection, const Item &item) {
    std::visit([&item](auto &value) { value.item = item; }, projection);
}

inline std::optional<ItemRecord28> captureBoundedItem(
    const BoundedItemProjection &projection, const ItemTable &table, GameTitle title) {
    return std::visit([&table, title](const auto &value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, PlainItemProjection>)
            return capturePlainItem(value, table, title);
        else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, StandardItemProjection>)
            return captureStandardItem(value, table, title);
        else
            return captureAntiqueItem(value, table, title);
    }, projection);
}

inline ItemType boundedItemSourceType(const BoundedItemProjection &projection) {
    return std::visit([](const auto &value) { return value.sourceType; }, projection);
}

} // namespace mm6
